/**
 * Connect to wifi as instructions from ESP-IDF
 * - [Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html#esp32-wi-fi-station-general-scenario)
 * - [Example](https://github.com/espressif/esp-idf/blob/master/examples/wifi/getting_started/station/main/station_example_main.c)
 */

#ifndef __WIFI_H__
#define __WIFI_H__

/// @brief Initialize wifi station and connect to wifi access point
/// @param ssid access point ssid
/// @param pass access point password
void wifi_sta_init(char * ssid, char * pass);

/// @brief Initialize wifi access point
/// @param ssid access point ssid
/// @param pass access point password
void wifi_ap_init(char * ssid, char * pass);

void wifi_router_init(char * ap_ssid, char * ap_pass, char * sta_ssid, char * sta_pass);

#endif
