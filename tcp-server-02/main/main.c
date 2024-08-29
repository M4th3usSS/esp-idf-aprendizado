#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

// Menuconfig - WiFi
#define EXAMPLE_ESP_WIFI_SSID           CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS           CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY       CONFIG_ESP_MAXIMUM_RETRY

// Menuconfig - Socket
#define EXAMPLE_ESP_SOCKET_PORT         CONFIG_ESP_SOCKET_PORT
#define EXAMPLE_KEEPALIVE_IDLE          CONFIG_KEEPALIVE_IDLE 
#define EXAMPLE_KEEPALIVE_INTERVAL      CONFIG_KEEPALIVE_INTERVAL
#define EXAMPLE_KEEPALIVE_COUNT         CONFIG_KEEPALIVE_COUNT

// Tags de depuração:
static const char *TAG_WIFI_STA = "DEBUG - WIFI-STA";
static const char *TAG_SOCKET = "DEBUG - TCP-SOCKET";

// Grupos de Eventos:
static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT          BIT0
#define WIFI_FAIL_BIT               BIT1

// Struct socket clients
typedef struct {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    int sock_client;
}Struct_Socket_clients;

// Task - Manager clients connect
void task_socket_client_handle(void* pvParameters) {

    Struct_Socket_clients Struct_Socket_client_x = *(Struct_Socket_clients*)pvParameters;

    char rx_buffer[128];
    int keepAlive = 1;
    int keepIdle = EXAMPLE_KEEPALIVE_IDLE;
    int keepInterval = EXAMPLE_KEEPALIVE_INTERVAL;
    int keepCount = EXAMPLE_KEEPALIVE_COUNT;

    for (;;) {
        
        // Set tcp keepalive option
        setsockopt(Struct_Socket_client_x.sock_client, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(Struct_Socket_client_x.sock_client, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(Struct_Socket_client_x.sock_client, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(Struct_Socket_client_x.sock_client, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));

        // receive data on the connected socket
        int len = recv(Struct_Socket_client_x.sock_client, rx_buffer, sizeof(rx_buffer) - 1, 0);

        if (len < 0) {
            ESP_LOGE(TAG_SOCKET, "[CLIENT-%d] Recv falhou: errno %d",Struct_Socket_client_x.sock_client, errno);
            break;
        } else

        if (len == 0) {
            ESP_LOGI(TAG_SOCKET, "[CLIENT-%d] Conexão fechada", Struct_Socket_client_x.sock_client);
            break;
        } 
        
        else {
            rx_buffer[len] = 0; // Null-terminate buffer
            ESP_LOGI(TAG_SOCKET, "[CLIENT-%d] Recebidos [%d bytes] : %s",Struct_Socket_client_x.sock_client , len, rx_buffer);

            // Echo back to sender
            int err = send(Struct_Socket_client_x.sock_client, rx_buffer, len, 0);
            if (err < 0) {
                ESP_LOGE(TAG_SOCKET, "[CLIENT-%d] Send falhou: errno %d", Struct_Socket_client_x.sock_client, errno);
                break;
            }
        }
    }

    if (Struct_Socket_client_x.sock_client != -1)
    {
        ESP_LOGE(TAG_WIFI_STA, "2- Shutting down socket and restarting...");
        shutdown(Struct_Socket_client_x.sock_client, 0);
        close(Struct_Socket_client_x.sock_client);
    }
    vTaskDelete(NULL);
    
}

// Task = Socket TCP/IP Server
void task_tcp_server(void* pvParametres) {

    char addr_str[128];

    int socket_fammily = (int)pvParametres;
    int socket_type = SOCK_STREAM;
    int socket_protocol = IPPROTO_TCP;

    // Creating a socket
    int socket_01 = socket(socket_fammily, socket_type, socket_protocol);
    if (socket_01 == -1) {
        ESP_LOGE(TAG_SOCKET, "Não foi possível criar o Socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }

    int opt = 1;

    // Set the socket options
    setsockopt(socket_01, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Log - Debug
    ESP_LOGI(TAG_SOCKET, "Socket criado");

    int err;
    
    struct sockaddr_in socket_adr = {
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_family = socket_fammily,
        .sin_port = htons(EXAMPLE_ESP_SOCKET_PORT)
    };
    
    // Binding a Server Socket
    err = bind(socket_01, (struct sockaddr *)&socket_adr, sizeof(socket_adr));
    if (err != 0) {
        ESP_LOGE(TAG_SOCKET, "Socket incapaz de vincular: %d", errno);
        ESP_LOGE(TAG_SOCKET, "IPPROTO: %d", socket_fammily);
        close(socket_01);
        vTaskDelete(NULL);
        return;
    }

    // Listening for Connections
    err = listen(socket_01, 3); 
    if (err != 0){
        ESP_LOGE(TAG_SOCKET, "Ocorreu um erro durante a escuta: errno %d", errno);
        close(socket_01);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG_SOCKET, "Escutando na porta %d", EXAMPLE_ESP_SOCKET_PORT);

    for(;;) {
        Struct_Socket_clients Struct_Socket_client_x;
        Struct_Socket_client_x.client_addr_len = sizeof(Struct_Socket_client_x.client_addr);
        
        Struct_Socket_client_x.sock_client = accept(socket_01, (struct sockaddr *)&Struct_Socket_client_x.client_addr, &Struct_Socket_client_x.client_addr_len);
        if (Struct_Socket_client_x.sock_client < 0) {
            ESP_LOGE(TAG_SOCKET, "Não foi possível aceitar a conexão: errno %d", errno);
            break;
        }

        // Convert ip addres to string
        if (Struct_Socket_client_x.client_addr.sin_family == PF_INET) {
            inet_ntoa_r(((struct sockaddr_in *)&Struct_Socket_client_x.client_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        }
        // Log - Debug
        ESP_LOGI(TAG_SOCKET, "[CLIENT-%d] Endereço IP aceito pelo Socket: %s",Struct_Socket_client_x.sock_client, addr_str);
        
        // Task multclientes
        xTaskCreate(task_socket_client_handle, "socket_client_handle", 4096, &Struct_Socket_client_x, 5, NULL);
    }

    // Task error
    if (socket_01 != -1) {
        ESP_LOGE(TAG_SOCKET, "1 - Desligando o soquete e reiniciando...");
        shutdown(socket_01, 0);
        close(socket_01);
    }
    vTaskDelete(NULL);
}

// Function - Event handlers
void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    
    int s_retry_num = (int) arg;

    // Event - STA Start
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else 

    // Event - Wifi disconnected
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG_WIFI_STA, " Repetindo conexão com o AP...");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else 

    // Event - Got-ip
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG_WIFI_STA, "IP recebido - " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }    
}

// Function - Started Wifi-STA
void init_wifi_sta(void) {

    static int s_retry_num = 0;

    // Creates default WIFI STA. In case of any init error this API aborts.
    esp_netif_create_default_wifi_sta();

    // Initialize WiFi
    wifi_init_config_t cfg_init_wifi = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg_init_wifi));

    // Register an instance of event handler to the default loop.
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        &s_retry_num,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        &s_retry_num,
                                                        &instance_got_ip));

    // Configuration data for device sta
    wifi_config_t wifi_cfg = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
        },
    };

    // Set the WiFi operating mode - WIFI_MODE_STA
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // Set the configuration of the STA
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));

    // Start WiFi
    ESP_ERROR_CHECK(esp_wifi_start());

    // Log - Debug
    ESP_LOGI(TAG_WIFI_STA, "Inicializado!");

    // Wait s_wifi_event_group
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                            pdFALSE,
                                            pdFALSE,
                                            portMAX_DELAY);

    if(bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG_WIFI_STA, "Conectado no ap  - ssid:%s, password:%s", EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else

    if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG_WIFI_STA, "Falha ao conecta no ap -  ssid:%s, password:%s", EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG_WIFI_STA, "Evento inesperado!");
    }

}

// App main
void app_main(void) {

    // Initialize the default NVS partition
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // Criando event-groups
    s_wifi_event_group = xEventGroupCreate();

    // Initialize the underlying TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Log - Debug
    ESP_LOGI(TAG_WIFI_STA, "Iniciando WiFi em modo estação");

    // Initialize wifi-sta
    init_wifi_sta();

    xTaskCreate(task_tcp_server, "tcp_server", 4096, (void*) PF_INET, 5, NULL);
}