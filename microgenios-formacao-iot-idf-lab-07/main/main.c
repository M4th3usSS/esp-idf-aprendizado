/******************************************************************************
 * Projeto:      microjenios-formacao-iot-idf-lab-07
 * Arquivo:      main.c
 * Autor:        Matheus Sousa Silva
 * Data:         22/08/2024
 * Descrição:    Estudo do código de exemplo de wifi-sta
 *
 * Plataforma:   ESP32
 * SDK:          ESP-IDF v5.3.0
 *
 * Dependências: Nenhuma
 *
 * Histórico de Modificações:
 * ----------------------------------------------------------------------------
 * 22/08/2024  |  Matheus Sousa |  Comentando o código
 * ----------------------------------------------------------------------------
 *
 * Notas:
 * - Foi necessário refatorar todo o código, baseado nos exemplos do curso que 
 *   estou fazendo, pois a versão atual do SDK é a 5.3.0. O curso de baseada 
 *   da versão 3.x.x
 *
 ******************************************************************************/


#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

/* Os exemplos usam configurações que você pode definir através do menu de configuração do projeto */
#define EXAMPLE_ESP_WIFI_SSID               CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS               CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY           CONFIG_ESP_MAXIMUM_RETRY

#if CONFIG_ESP_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE                   WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER              ""
#elif CONFIG_ESP_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE                   WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EXAMPLE_H2E_IDENTIFIER              CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE                   WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER              CONFIG_ESP_WIFI_PW_ID
#endif
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WAPI_PSK
#endif

/* Grupo de eventos FreeRTOS para sinalizar quando estamos conectados */
static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi station";

static int s_retry_num = 0;

/* event_handler é usada para gerenciar os principais eventos de conexão Wi-Fi no ESP32

    Essa função event_handler é usada para gerenciar os principais eventos de conexão Wi-Fi no ESP32. Ela tenta reconectar automaticamente 
    em caso de desconexão e registra os estados da conexão, como sucesso ou falha, usando grupos de eventos e logs. Este é um padrão comum 
    para lidar com a conectividade Wi-Fi em aplicações embarcadas usando o ESP-IDF.
*/
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    
    /* Tratamento do Evento WIFI_EVENT_STA_START

        Quando a base do evento é WIFI_EVENT e o evento específico é WIFI_EVENT_STA_START, a função chama esp_wifi_connect() para 
        tentar conectar à rede Wi-Fi configurada.
    */
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else 
    /* Tratamento do Evento WIFI_EVENT_STA_DISCONNECTED

        Esse bloco trata o evento WIFI_EVENT_STA_DISCONNECTED, que é acionado quando a conexão Wi-Fi é perdida.
        Se o número de tentativas de reconexão (s_retry_num) for menor que o máximo permitido (EXAMPLE_ESP_MAXIMUM_RETRY), ele 
        tenta reconectar chamando esp_wifi_connect() novamente e incrementa s_retry_num.
        Se o número de tentativas exceder o máximo permitido, o código define um bit em um grupo de eventos (s_wifi_event_group) 
        para indicar a falha na conexão (WIFI_FAIL_BIT).
        Logs informativos (ESP_LOGI) são usados para relatar as tentativas de reconexão e falhas.

    */
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    
    } else 
    /* Tratamento do Evento IP_EVENT_STA_GOT_IP

        Esse bloco é acionado quando o evento IP_EVENT_STA_GOT_IP ocorre, indicando que o dispositivo obteve um endereço IP válido.
        A função extrai o endereço IP dos dados do evento (event_data) e o imprime nos logs.
        O contador de tentativas de reconexão (s_retry_num) é resetado para 0, pois a conexão foi bem-sucedida.
        Um bit é definido no grupo de eventos (s_wifi_event_group) para indicar que a conexão foi estabelecida com sucesso 
        (WIFI_CONNECTED_BIT).
    */
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/* Função responsável por configurar e inicializar a interface Wi-Fi em modo station (STA)

    A função também configura o tratamento de eventos relacionados à conexão Wi-Fi, esperando 
    até que a conexão seja estabelecida com sucesso ou que as tentativas de conexão sejam esgotadas.
*/
void wifi_init_sta(void) {

    /* Inicialização da Interface de Rede

    A função esp_netif_init() inicializa a camada de interface de rede do ESP-IDF. Essa camada é responsável por gerenciar as interfaces de rede, como 
    Wi-Fi e Ethernet.
    */
    ESP_ERROR_CHECK(esp_netif_init());

    /* Criação do Loop de Eventos

        esp_event_loop_create_default() cria um loop de eventos padrão, que será utilizado para lidar com eventos do sistema.
    */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Registro dos Manipuladores de Eventos

        Dois manipuladores de eventos são registrados:
        O primeiro (WIFI_EVENT) captura qualquer evento relacionado ao Wi-Fi (usando ESP_EVENT_ANY_ID).
        O segundo (IP_EVENT) captura especificamente o evento IP_EVENT_STA_GOT_IP, que indica que o dispositivo obteve um endereço IP.

        Esses eventos são tratados pela função event_handler.
    */
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

    /* Cria uma interface de rede Wifi

        esp_netif_create_default_wifi_sta() cria uma interface de rede Wi-Fi padrão configurada para o modo station (STA), que permite ao ESP32 conectar-se 
        a um ponto de acesso.
    */
    esp_netif_create_default_wifi_sta();

    /* Inicialização do Wi-Fi

        Aqui, a função esp_wifi_init() inicializa a pilha Wi-Fi com as configurações padrão definidas em WIFI_INIT_CONFIG_DEFAULT().
    */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Configuração da Interface Wi-Fi

        wifi_config_t wifi_config: Define as configurações da interface STA, incluindo o SSID e a senha do ponto de acesso.
        esp_wifi_set_mode(WIFI_MODE_STA): Configura o ESP32 para o modo station.
        esp_wifi_set_config(WIFI_IF_STA, &wifi_config): Aplica as configurações Wi-Fi definidas.
        esp_wifi_start(): Inicia a interface Wi-Fi, o que dispara o evento WIFI_EVENT_STA_START.
    */
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Configurações avançadas 

                Authmode Threshold: Por padrão, o ESP32 configura o modo de autenticação (authmode) para WPA2 se a senha fornecida tiver 8 ou 
                mais caracteres. Isso ocorre porque o WPA2 é considerado mais seguro e é amplamente utilizado em redes Wi-Fi modernas.

                Redes WEP/WPA Desatualizadas: Se o usuário deseja conectar o dispositivo a uma rede Wi-Fi que usa os modos de segurança mais antigos 
                e menos seguros, como WEP ou WPA (que é diferente de WPA2), ele deve ajustar explicitamente o parâmetro threshold.authmode para 
                WIFI_AUTH_WEP ou WIFI_AUTH_WPA_PSK. Além disso, a senha deve ser configurada de acordo com os requisitos específicos desses modos 
                (por exemplo, comprimento e formato).
            */
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
            .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Aguardando Conexão ou Falha 

        xEventGroupWaitBits() espera até que um dos bits de evento seja definido (WIFI_CONNECTED_BIT ou WIFI_FAIL_BIT), indicando que a conexão foi 
        estabelecida ou falhou.
        portMAX_DELAY faz com que a função espere indefinidamente até que um dos eventos ocorra.
    */
    EventBits_t eventBits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    /* Verificação do Resultado

        O código verifica qual bit foi definido e imprime um log correspondente:
        Se WIFI_CONNECTED_BIT foi definido, indica que a conexão foi bem-sucedida.
        Se WIFI_FAIL_BIT foi definido, indica que a conexão falhou após o número máximo de tentativas.
        Qualquer outro caso é tratado como um evento inesperado. 
    */
    if (eventBits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (eventBits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

}

/* Função principal */
void app_main(void) {
   
    /*  Inicialização do NVS (Non-Volatile Storage)
       
        nvs_flash_init(): Esta função inicializa o NVS, que é uma partição da memória flash do ESP32 usada para armazenar dados que precisam 
        persistir entre reinicializações, como configurações de Wi-Fi, contadores, e outras informações de configuração.

        Verificação de Erros:

        A função retorna um código de erro esp_err_t que indica o status da inicialização.
        Se o retorno for ESP_ERR_NVS_NO_FREE_PAGES ou ESP_ERR_NVS_NEW_VERSION_FOUND, isso significa que a partição NVS está cheia ou que uma nova 
        versão da NVS foi encontrada, exigindo uma reconfiguração.
        
        nvs_flash_erase(): Caso uma dessas duas condições seja verdadeira, a função apaga o conteúdo da NVS para liberar espaço ou permitir a 
        atualização da estrutura de dados.

        Reinicialização do NVS: Após apagar a NVS, a função nvs_flash_init() é chamada novamente para garantir que a NVS seja inicializada 
        corretamente.

        ESP_ERROR_CHECK(ret): Essa macro verifica se a inicialização do NVS foi bem-sucedida após a possível reconfiguração. Se houve falha, o 
        código é interrompido e uma mensagem de erro é gerada.
    */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase()); // Apaga toda memória não volátil 
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Criação de um Grupo de Eventos
        
        Um grupo de eventos é criado usando xEventGroupCreate(). Esse grupo (s_wifi_event_group) será usado para sinalizar eventos como sucesso ou falha 
        na conexão Wi-Fi.
    */
    s_wifi_event_group = xEventGroupCreate();

    /* Log de Modo Wi-Fi
        
        ESP_LOGI(TAG, "ESP_WIFI_MODE_STA"): Essa linha imprime uma mensagem no log indicando que o modo Wi-Fi station (STA) está prestes a ser 
        configurado e inicializado. TAG é geralmente uma string que identifica o módulo ou componente que está emitindo o log, e ESP_LOGI é uma 
        função de log de informações.
    */
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");

    /* Esta função configura e inicia o Wi-Fi em modo station (STA). 
    
        Isso inclui:

        - Criação de um grupo de eventos.
        - Inicialização da interface de rede.
        - Configuração dos parâmetros Wi-Fi (SSID e senha).
        - Registro de manipuladores de eventos para tratar conexão, desconexão e obtenção de IP.
        - Tentativa de conexão ao ponto de acesso especificado.
        - Espera até que a conexão seja estabelecida ou que todas as tentativas falhem.
    */
    wifi_init_sta();
}