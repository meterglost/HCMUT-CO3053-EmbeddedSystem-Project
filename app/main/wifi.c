/**
 * Connect to wifi as instructions from ESP-IDF
 * - [Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html#esp32-wi-fi-station-general-scenario)
 * - [Example](https://github.com/espressif/esp-idf/blob/master/examples/wifi/getting_started/station/main/station_example_main.c)
 */

#include <string.h>
#include "sdkconfig.h"
#include "esp_system.h"
#include "esp_check.h"

#include "esp_log.h"
#include "esp_event.h"

#include "esp_netif.h"
#include "esp_netif_net_stack.h"

#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/lwip_napt.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_mac.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "wifi.h"

// --- WIFI CONFIG

void wifi_config_storage_init()
{
    ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_ERROR_CHECK(nvs_flash_init());
}

void wifi_config_init()
{
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_netif_init());

    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));
}

// --- WIFI STATION

void wifi_sta_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    static int s_retry_times = 0;

    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
        s_retry_times = 5;
        esp_wifi_connect();
        break;

    case WIFI_EVENT_STA_CONNECTED:
        s_retry_times = 5;
        break;

    case WIFI_EVENT_STA_DISCONNECTED:
        if (s_retry_times > 0) {
            ESP_LOGW("wifi", "Retrying to connect to wifi...");
            --s_retry_times;
            esp_wifi_connect();
        } else {
            ESP_LOGE("wifi", "Failed to connect to wifi...");
        }
        break;

    case WIFI_EVENT_STA_STOP:
        s_retry_times = 0;
        break;
    }
}

esp_netif_t * wifi_sta_config(char * ssid, char * pass)
{
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_STOP, &wifi_sta_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, &wifi_sta_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &wifi_sta_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &wifi_sta_event_handler, NULL));

    esp_netif_t * esp_netif_sta = esp_netif_create_default_wifi_sta();

    wifi_config_t wifi_config = { .sta = { } };
    strncpy((char *) wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *) wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

    return esp_netif_sta;
}

void wifi_sta_init(char * ssid, char * pass)
{
    wifi_config_storage_init();
    wifi_config_init();
    wifi_sta_config(ssid, pass);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

// --- WIFI ACCESS POINT

void wifi_ap_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    switch (event_id)
    {
    case WIFI_EVENT_AP_STACONNECTED:
        wifi_event_ap_staconnected_t* event_connect = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI("wifi", "station "MACSTR" join, AID=%d", MAC2STR(event_connect->mac), event_connect->aid);
        break;

    case WIFI_EVENT_AP_STADISCONNECTED:
        wifi_event_ap_stadisconnected_t* event_disconnect = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI("wifi", "station "MACSTR" leave, AID=%d", MAC2STR(event_disconnect->mac), event_disconnect->aid);
        break;
    }
}

esp_netif_t * wifi_ap_config(char * ssid, char * pass)
{
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &wifi_ap_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &wifi_ap_event_handler, NULL));

    esp_netif_t * esp_netif_ap = esp_netif_create_default_wifi_ap();

    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));

    wifi_config_t wifi_config = { .ap = { .max_connection = 4, .authmode = WIFI_AUTH_WPA3_PSK } };
    strncpy((char *) wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid));
    strncpy((char *) wifi_config.ap.password, pass, sizeof(wifi_config.ap.password));

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));

    return esp_netif_ap;
}

void wifi_ap_init(char * ssid, char * pass)
{
    wifi_config_storage_init();
    wifi_config_init();
    wifi_ap_config(ssid, pass);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_start());
}

// --- WIFI ROUTER

void wifi_router_init(char * ap_ssid, char * ap_pass, char * sta_ssid, char * sta_pass)
{
    wifi_config_storage_init();
    wifi_config_init();

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    esp_netif_t *esp_netif_sta = wifi_sta_config(sta_ssid, sta_pass);
    esp_netif_t *esp_netif_ap = wifi_ap_config(ap_ssid, ap_pass);

    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_netif_set_default_netif(esp_netif_sta));
    ESP_ERROR_CHECK(esp_netif_napt_enable(esp_netif_ap));
}
