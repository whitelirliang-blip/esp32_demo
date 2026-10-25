/*
 * SPDX-FileCopyrightText: 2026 Xiquan Liang
 * SPDX-License-Identifier: MIT
 */
#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include "mqtt_client.h"

void mqtt_app_start(void);
esp_err_t mqtt_manager_publish(const char *topic, const char *data, int len, int qos, int retain);

#endif 