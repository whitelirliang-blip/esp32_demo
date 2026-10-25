/*
 * SPDX-FileCopyrightText: 2026 Xiquan Liang
 * SPDX-License-Identifier: MIT
 */
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

void wifi_init_sta(void);

#endif