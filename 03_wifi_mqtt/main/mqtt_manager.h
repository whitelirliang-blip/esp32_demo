/*
 * SPDX-FileCopyrightText: 2026 Xiquan Liang
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize and start the MQTT client service
 */
void mqtt_app_start(void);

/**
 * @brief Thread-safe public API to publish telemetry or control messages
 * 
 * @param topic MQTT topic string
 * @param data Payload buffer pointer
 * @param len Payload length in bytes
 * @param qos Quality of Service (0, 1, or 2)
 * @param retain Retain flag (0 or 1)
 * @return esp_err_t ESP_OK on success, or appropriate error code
 */
esp_err_t mqtt_manager_publish(const char *topic, const char *data, int len, int qos, int retain);

#ifdef __cplusplus
}
#endif