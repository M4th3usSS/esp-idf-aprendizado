/*
	----------------------------------------------------------
	GPIO_MODE_OUTPUT       //Saída
	GPIO_MODE_INPUT        //Entrada
	GPIO_MODE_INPUT_OUTPUT //Dreno Aberto
	----------------------------------------------------------
    GPIO_PULLUP_ONLY,               // Pad pull up            
    GPIO_PULLDOWN_ONLY,             // Pad pull down          
    GPIO_PULLUP_PULLDOWN,           // Pad pull up + pull down
    GPIO_FLOATING,                  // Pad floating  
    ----------------------------------------------------------
*/

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

#define LED     GPIO_NUM_12
#define BUTTON  GPIO_NUM_14

void vTaksLed(void *Parameters) {

    /* Configura a GPIO como modo OUTPUT */
    esp_rom_gpio_pad_select_gpio(LED);
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);

    /* Condigura a GPIO como modo INPUT PULLUP ONLY*/
    esp_rom_gpio_pad_select_gpio(BUTTON);
    gpio_set_direction(BUTTON, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON, GPIO_PULLUP_ONLY);

    printf("Pisca LED com Botão\n");

    for(;;) {

        static uint8_t ucCounter = 0;

        if (gpio_get_level(BUTTON) == 0) {
            gpio_set_level(LED, ucCounter % 2);
            ucCounter++;
        } else {
            gpio_set_level(LED, false);
        }
        
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

void app_main(void) {
    xTaskCreate(vTaksLed, "Task-Led", 1024, NULL, 1, NULL);
    printf("Task LED iniciada com sucesso!\n");
}