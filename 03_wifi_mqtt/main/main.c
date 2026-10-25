/*
 * SPDX-FileCopyrightText: 2026 Xiquan Liang
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"

static const char *TAG = "main";

#define TELEMETRY_STACK_SIZE 4096
#define TELEMETRY_PERIOD_MS  5000

static void vTaskTelemetry(void *pvParameters)
{
    char payload[128];
    uint32_t counter = 0;

    // Wait until network is up
    vTaskDelay(pdMS_TO_TICKS(4000));

    while (1) {
        snprintf(payload, sizeof(payload), "{\"device_id\":\"esp32_s3\",\"seq\":%lu,\"uptime\":%lu}", 
                 counter++, (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS));
        
        ESP_LOGI(TAG, "Publishing telemetry: %s", payload);
        mqtt_manager_publish("iot/esp32/telemetry", payload, 0, 1, 0);

        vTaskDelay(pdMS_TO_TICKS(TELEMETRY_PERIOD_MS));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing NVS Flash...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Initializing Wi-Fi Station...");
    wifi_init_sta();

    ESP_LOGI(TAG, "Starting MQTT Client...");
    mqtt_app_start();

    ESP_LOGI(TAG, "Spawning Telemetry Worker Task...");
    xTaskCreate(&vTaskTelemetry, "vTaskTelemetry", TELEMETRY_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
}