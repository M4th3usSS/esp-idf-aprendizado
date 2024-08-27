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
#define GPIO_OUTPUT_PIN_SEL ((1ULL<<LED_1) | (1ULL<<LED_2));

#define BUTTON_1 GPIO_NUM_14
#define BUTTON_2 GPIO_NUM_27
#define GPIO_INPUT_PIN_SEL ((1ULL<<BUTTON_1) | (1ULL<<BUTTON_2));

void Task_LED(void *pvParameters) {

    gpio_config_t io_conf;

    // Configura o descritor de Outputs(LEDs)
    io_conf.intr_type = GPIO_INTR_DISABLE;          //Desabilita o recurso de interrupção neste descritor.
    io_conf.mode = GPIO_MODE_OUTPUT;                //Configura como saída. 
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;     //Informa quais os pinos que serão configurados pelo drive.           
    gpio_config(&io_conf);                          //Configura a(s) GPIO's conforme configuração do descritor.

    // Configura o descritor das Inputs(Botões)
    io_conf.intr_type = GPIO_INTR_DISABLE;          //Desabilita o recurso de interrupção neste descritor.
    io_conf.mode = GPIO_MODE_INPUT;                 //Configura como saída. 
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;      //Informa quais os pinos que serão configurados pelo drive.
    io_conf.pull_down_en = GPIO_PULLUP_DISABLE;     //ou  GPIO_PULLDOWN_ENABLE
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;        //ou GPIO_PULLUP_DISABLE
    gpio_config(&io_conf);                          //Configura a(s) GPIO's conforme configuração do descritor.

    printf("Pisca LED_1 e LED_2\n");

    for(;;) {

        static uint8_t ucCounter = 0;

        if ((gpio_get_level(BUTTON_1) == 0) || (gpio_get_level(BUTTON_2) == 0)) {
            gpio_set_level(LED_1, ucCounter % 2);
            gpio_set_level(LED_2, ucCounter % 2);
            ucCounter++;
        } else {
            gpio_set_level(LED_1, false);
            gpio_set_level(LED_2, false);
            ucCounter = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

void app_main(void) {
    xTaskCreate(Task_LED, "Task-LED", 2048, NULL, 1, NULL);
    printf("Task-LED iniciada com sucesso!\n");
}