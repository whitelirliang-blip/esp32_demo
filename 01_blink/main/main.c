#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define LED_PIN GPIO_NUM_2

static const char *TAG = "BLINK";

void app_main(void) {
    // Equivalent to setup(): Configure GPIO pin modes
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    // Equivalent to loop(): enters an infinite loop
    while (1) {
        gpio_set_level(LED_PIN, 1);
        ESP_LOGI(TAG, "LED ON");
        vTaskDelay(pdMS_TO_TICKS(1000));

        gpio_set_level(LED_PIN, 0);
        ESP_LOGI(TAG, "LED OFF");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}