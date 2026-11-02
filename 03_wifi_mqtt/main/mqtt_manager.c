/*
 * SPDX-FileCopyrightText: 2026 Xiquan Liang
 * SPDX-License-Identifier: MIT
 */

#include "mqtt_manager.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_err.h"
#include "esp_event.h"

static const char *TAG __attribute__((unused)) = "mqtt_manager";
static esp_mqtt_client_handle_t client = NULL;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    if (event == NULL) {
        ESP_LOGE(TAG, "Event data pointer is NULL");
        return;
    }

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED to broker at %s", CONFIG_ESP_MQTT_BROKER_URI);
            
            // 建立链路后自动订阅下行控制主题
            int sub_id = esp_mqtt_client_subscribe(client, "esp32/control", 0);
            if (sub_id < 0) {
                ESP_LOGE(TAG, "Failed to subscribe to control topic");
            } else {
                ESP_LOGI(TAG, "Successfully subscribed to 'esp32/control', msg_id=%d", sub_id);
            }
            
            // 上报初始化遥测帧
            esp_mqtt_client_publish(client, "sensor/telemetry", "{\"status\":\"online\"}", 0, 1, 0);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT_EVENT_DISCONNECTED from broker");
            break;

        case MQTT_EVENT_DATA:
            // 安全处理下行字节流（注意：event->data 不以 \0 结尾）
            ESP_LOGI(TAG, "MQTT_EVENT_DATA received on topic %.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "Payload: %.*s", event->data_len, event->data);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGD(TAG, "MQTT_EVENT_PUBLISHED successful, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR encountered");
            if (event->error_handle != NULL) {
                ESP_LOGE(TAG, "Last error type: 0x%x", event->error_handle->error_type);
            }
            break;

        default:
            break;
    }
}

void mqtt_app_start(void)
{
    #ifndef CONFIG_ESP_MQTT_BROKER_URI
    #error "CONFIG_ESP_MQTT_BROKER_URI is not defined in sdkconfig!"
    #endif

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_ESP_MQTT_BROKER_URI,
    };

    if (client != NULL) {
        ESP_LOGW(TAG, "MQTT client already initialized");
        return;
    }

    client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return;
    }

    ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mqtt_client_start(client));
}

esp_err_t mqtt_manager_publish(const char *topic, const char *data, int len, int qos, int retain)
{
    if (client == NULL || topic == NULL || data == NULL) {
        ESP_LOGW(TAG, "Publish aborted: invalid arguments or uninitialized client");
        return ESP_ERR_INVALID_ARG;
    }

    int msg_id = esp_mqtt_client_publish(client, topic, data, len, qos, retain);
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to enqueue MQTT message");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}