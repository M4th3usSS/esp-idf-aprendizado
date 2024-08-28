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
#include "protocol_examples_common.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#define PORT CONFIG_EXAMPLE_PORT
#define KEEPALIVE_IDLE CONFIG_EXAMPLE_KEEPALIVE_IDLE
#define KEEPALIVE_INTERVAL CONFIG_EXAMPLE_KEEPALIVE_INTERVAL
#define KEEPALIVE_COUNT CONFIG_EXAMPLE_KEEPALIVE_COUNT

#define MAX_CLIENTS 5

static const char *TAG = "TCP-SERVER-SOCKET";
static int client_count = 0;                    // Contador de clientes conectados
static SemaphoreHandle_t client_mutex;          // Mutex para proteger o contador de clientes

void handle_client_task(void *pvParameters) 
{
    int client_sock = (int)pvParameters;
    char rx_buffer[128];

    while (1)
    {
        int len = recv(client_sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        
        if (len < 0) {
            ESP_LOGE(TAG, "Recv failed: errno %d", errno);
            break;
        }
        else if (len == 0)
        {
            ESP_LOGI(TAG, "Connection closed");
            break;
        }
        else
        {
            rx_buffer[len] = 0; // Null-terminate buffer
            ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);

            // Echo back to sender
            int err = send(client_sock, rx_buffer, len, 0);
            if (err < 0)
            {
                ESP_LOGE(TAG, "Send failed: errno %d", errno);
                break;
            }
        }
    }

    if (client_sock != -1)
    {
        ESP_LOGE(TAG, "Shutting down socket and restarting...");
        shutdown(client_sock, 0);
        close(client_sock);
    }

    // Protege a operação de decrementar o contador de clientes com o mutex
    xSemaphoreTake(client_mutex, portMAX_DELAY);
    client_count--;  // Decrementa o número de clientes conectados
    xSemaphoreGive(client_mutex);

    vTaskDelete(NULL);
}

static void tcp_server_task(void *pvParameters) 
{

    char addr_str[128];
    int domain = AF_INET;
    int type = SOCK_STREAM;
    int protocol = IPPROTO_IP;

    struct sockaddr_in dest_addr = {
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
    };

    /* Criando um Socket */
    int listen_sock = socket(domain, type, protocol);

    if (listen_sock < 0)
    {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }

    /* Configurando o Socket */
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    ESP_LOGI(TAG, "Socket created");

    /* Associando o socket a um endereço de rede e a uma porta */
    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    
    if (err != 0)
    {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(TAG, "IPPROTO: %d", domain);
        return;
    }

    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    /* Marcando socket como passivo, ou seja, um socket que será usado para aceitar conexões de entrada*/
    err = listen(listen_sock, 1);
    
    if (err != 0)
    {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        return;
    }

    while (1)
    {

        ESP_LOGI(TAG, "Waiting for new connection...");

        // Protege a operação de leitura do contador de clientes com o mutex
        xSemaphoreTake(client_mutex, portMAX_DELAY);
        if (client_count >= MAX_CLIENTS) {
            xSemaphoreGive(client_mutex);    // Libera o mutex antes de continuar
            ESP_LOGW(TAG, "Max clients reached, waiting for a slot...");
            vTaskDelay(pdMS_TO_TICKS(1000)); // Espera antes de tentar novamente
            continue;
        }
        xSemaphoreGive(client_mutex);  // Libera o mutex após a verificação

        /* Criando Cliente-Socket */
        struct sockaddr_storage source_addr;
        socklen_t source_addr_len = sizeof(source_addr);
        int client_sock = accept(listen_sock, (struct sockaddr *)&source_addr, &source_addr_len);

        if (client_sock < 0)
        {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

        // Incrementa o contador de clientes conectados protegendo com o mutex
        xSemaphoreTake(client_mutex, portMAX_DELAY);
        client_count++;
        xSemaphoreGive(client_mutex);

        // Cria uma nova tarefa para lidar com o cliente conectado
        xTaskCreate(handle_client_task, "handle_client", 4096, (void *)client_sock, 5, NULL);
    }

    if (listen_sock != -1)
    {
        ESP_LOGI(TAG, "Shutting down socket and restarting...");
        shutdown(listen_sock, 0);
        close(listen_sock);
    }
    vTaskDelete(NULL);
}

void app_main(void) {

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    client_mutex = xSemaphoreCreateMutex();
    if (client_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create client mutex");
        return;
    }

    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);
}
