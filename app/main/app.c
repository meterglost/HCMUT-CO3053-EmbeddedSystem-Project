/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "sdkconfig.h"
#include "esp_system.h"

// Libraries needed for Network
#include "wifi.h"

// Libraries needed for Webserver
#include "web.h"

// Libraries needed for Sensor
#include "sensor.h"

#define AP_SSID "ESP32-AP-TEST"
#define AP_PASS "12345678"

#define STA_SSID ""
#define STA_PASS ""

void app_main(void)
{
    wifi_router_init(AP_SSID, AP_PASS, STA_SSID, STA_PASS);
    init_webserver();
    init_sensor();
}
