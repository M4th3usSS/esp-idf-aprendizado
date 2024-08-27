/*
  Autor: Matheus Sousa Silva
  Hardware: ESP32
  SDK-IDF: v5.3.0
  Curso Online: Formação em Internet das Coisas (IoT) com ESP32
*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

/*
 Pisca um LED conectado no pino GPIO_12 do ESP32.
 Esquema Oficial: https://dl.espressif.com/dl/schematics/ESP32-Core-Board-V2_sch.pdf
*/

void blink_task(void *pvParameter) {
    esp_rom_gpio_pad_select_gpio(GPIO_NUM_12); 
    gpio_set_direction(GPIO_NUM_12,GPIO_MODE_OUTPUT);
    printf("Blinking LED on GPIO 16\n");
    
    uint16_t usCoubter = 0;
    
    while(1) {
		gpio_set_level(GPIO_NUM_12,usCoubter % 2); // cnt % 2 (ou seja, se cnt for par, retorna 0, caso contrário, retorna 1)
        usCoubter++;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
 
void app_main() {	
    xTaskCreate(blink_task,"blink_task",1024,NULL,1,NULL);
    printf("blink task  started\n");
}