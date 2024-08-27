#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define LED_1 GPIO_NUM_12
#define LED_2 GPIO_NUM_13    
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<LED_1) | (1ULL<<LED_2))

#define BUTTON_1 GPIO_NUM_14 
#define BUTTON_2 GPIO_NUM_27
#define GPIO_INPUT_PIN_SEL  ((1ULL<<BUTTON_1) | (1ULL<<BUTTON_2))

#define ESP_INTR_FLAG_DEFAULT 0

volatile uint8_t ucCounter1 = 0;
volatile uint8_t ucCounter2 = 0;

/* Cada vez que Button_1 ou Button_2 for pressionado, esta função de IRQ será chamada*/
static void IRAM_ATTR gpio_isr_handler(void* arg) { 

    uint32_t ulButton = (uint32_t) arg;

    //Identifica qual o botão que foi pressionado
    if( BUTTON_1 == ulButton ) {
		//Caso o botão 1 estiver pressionado, faz a leitura e o acionamento do led.
		if( gpio_get_level( ulButton ) == 0 ) {
			gpio_set_level( LED_1, ucCounter1 % 2 );
		}
	} 
    else if( BUTTON_2 == ulButton ) {
        //Caso o botão 2 estiver pressionado, faz a leitura e o acionamento do led.
		if( gpio_get_level( ulButton ) == 0 ) {
			gpio_set_level( LED_2, ucCounter2 % 2 );
		}
	}   
	
	ucCounter1++;
	ucCounter2++;	
}

void Task_LED(void *pvParameters) {

    gpio_config_t io_conf1 = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = GPIO_OUTPUT_PIN_SEL
    };
    gpio_config(&io_conf1);

    gpio_config_t io_conf2 = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = GPIO_INPUT_PIN_SEL,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&io_conf2);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(BUTTON_1, gpio_isr_handler, (void*) BUTTON_1);
    gpio_isr_handler_add(BUTTON_2, gpio_isr_handler, (void*) BUTTON_2);

    printf("Pisca LED_1 e LED_2\n");

    for (;;) {
        // Aguardando a interrupção das GPIOs serem geradas...
        vTaskDelay(pdMS_TO_TICKS(300));
    }
    
}

void app_main(void) {
    xTaskCreate(Task_LED, "Task-LED", 2048, NULL, 2, NULL);
    printf("Task-LED iniciada com sucesso.\n");
}