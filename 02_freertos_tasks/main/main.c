/*
 * SPDX-FileCopyrightText: 2026 Xiquan Liang
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

// Hardware and timing macro definitions (magic numbers strictly prohibited)
#define LED_GPIO_PIN          GPIO_NUM_2
#define HEARTBEAT_PERIOD_MS   1000
#define TELEMETRY_PERIOD_MS   2500

// Task stack and priority configuration
#define HEARTBEAT_STACK_SIZE  2048
#define TELEMETRY_STACK_SIZE  3072
#define HEARTBEAT_PRIORITY    2
#define TELEMETRY_PRIORITY    1

static const char *TAG = "multi_task";

// Task 1: High-priority heartbeat and watchdog feeding
static void vTaskHeartbeat(void *pvParameters) {
gpio_reset_pin(LED_GPIO_PIN);
gpio_set_direction(LED_GPIO_PIN, GPIO_MODE_OUTPUT);

TickType_t xLastWakeTime = xTaskGetTickCount();
const TickType_t xFrequency = pdMS_TO_TICKS(HEARTBEAT_PERIOD_MS);

uint8_t led_level = 0;

while (1) {
led_level = !led_level;
gpio_set_level(LED_GPIO_PIN, led_level);
ESP_LOGI(TAG, "HEARTBEAT: LED State -> %d", led_level);

// Use absolute delay to ensure precise periodicity and avoid time drift
vTaskDelayUntil(&xLastWakeTime, xFrequency);
}
}

// Task 2: Medium/low-priority telemetry task
static void vTaskTelemetry(void *pvParameters) {
uint32_t sample_counter = 0;

while (1) {
sample_counter++;
// Log the remaining system heap memory to monitor for potential heap leaks
size_t free_heap = xPortGetFreeHeapSize();
ESP_LOGI(TAG, "TELEMETRY: Sample #%lu | Free Heap: %u bytes", sample_counter, (unsigned int)free_heap);

vTaskDelay(pdMS_TO_TICKS(TELEMETRY_PERIOD_MS));
}
}

void app_main(void) {
ESP_LOGI(TAG, "Initializing FreeRTOS Multi-Tasking Baseline...");

// Create the heartbeat task
BaseType_t hb_ret = xTaskCreate(
vTaskHeartbeat,
"HeartbeatTask",
HEARTBEAT_STACK_SIZE,
NULL,
HEARTBEAT_PRIORITY,
NULL
);
configASSERT(hb_ret == pdPASS);

// Create the telemetry task
BaseType_t tm_ret = xTaskCreate(
vTaskTelemetry,
"TelemetryTask",
TELEMETRY_STACK_SIZE,
NULL,
TELEMETRY_PRIORITY,
NULL
);
configASSERT(tm_ret == pdPASS);

ESP_LOGI(TAG, "All tasks spawned successfully. Main task exiting execution context.");
// Once app_main exits, the FreeRTOS scheduler continues to manage the background tasks that have been created.
}