#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

#define TIME_BLINK 1000

void vBlinkTask(void *Parameters) {

    esp_rom_gpio_pad_select_gpio(GPIO_NUM_12);
    gpio_set_direction(GPIO_NUM_12, GPIO_MODE_OUTPUT);
    printf("Blinking LED on GPIO 12\n");

    for(;;) {

        static uint8_t ucCounter = 0;

        gpio_set_level(GPIO_NUM_12, ucCounter % 2);
        ucCounter++;
        vTaskDelay(pdMS_TO_TICKS(TIME_BLINK));
    }
}

void app_main(void) {
    xTaskCreate(vBlinkTask, "Task-Blink", 1024, NULL, 1, NULL);
    printf("Blink task started\n");
}