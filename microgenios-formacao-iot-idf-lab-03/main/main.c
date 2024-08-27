/*
  Autor: Matheus Sousa
  Hardware: ESP32
  SDK-IDF: v5.3.0
  Curso Online: Formação em Internet das Coisas (IoT) com ESP32
  Link: https://www.microgenios.com.br/formacao-iot-esp32/
*/

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

#define LED_1 GPIO_NUM_12  
#define LED_2 GPIO_NUM_13 
#define GPIO_OUTPUT_PIN_OUTPUTS  ((1ULL<<LED_1) | (1ULL<<LED_2)) // Mascara de bits
 
void Task_LED(void *pvParameter) {
	
    gpio_config_t io_config;                          // Declara a variável descritora do drive de GPIO.
    
    io_config.intr_type = GPIO_INTR_DISABLE;          // Desabilita o recurso de interrupção neste descritor.
    io_config.mode = GPIO_MODE_OUTPUT;                // Configura como saída. 
    io_config.pin_bit_mask = GPIO_OUTPUT_PIN_OUTPUTS; // Informa quais os pinos que serão configurados pelo drive. 
	
	/*	
		Pull-Up e Pull-Down somente são aplicados quando os pinos forem configurados como entrada
		Portanto, os comandos a seguir permanecem comentados.
	*/

    //io_config.pull_down_en = GPIO_PULLDOWN_DISABLE; //ou  GPIO_PULLDOWN_ENABLE
    //io_config.pull_up_en = GPIO_PULLUP_DISABLE;  //ou GPIO_PULLUP_DISABLE
	
    gpio_config(&io_config); // Configura a(s) GPIO's conforme configuração do descritor. 
	
    printf("Pisca LED_1 e LED_2\n");
    
    for(;;) {
		
		static uint8_t ucCounter = 0;

		gpio_set_level(LED_1, ucCounter%2);
		gpio_set_level(LED_2, ucCounter%2);
		ucCounter++;	 
		vTaskDelay(pdMS_TO_TICKS(300));
    }
}
 
void app_main() {	
    xTaskCreate(Task_LED, "Task_LED", 2048, NULL, 2, NULL);
    printf("Task_LED iniciada com sucesso.\n");
}

