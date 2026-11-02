# SPDX-FileCopyrightText: 2026 Xiquan Liang
# SPDX-License-Identifier: MIT

idf_component_register(
    SRCS "main.c" "wifi_manager.c" "mqtt_manager.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_wifi esp_mqtt nvs_flash esp_netif
)