#include "sdkconfig.h"
#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sensor.h"

#include "esp_log.h"

#ifndef TEM_RMIN
    #error TEM_RMIN is not defined
#endif

#ifndef TEM_RMAX
    #error TEM_RMAX is not defined
#endif

#ifndef HUM_RMIN
    #error HUM_RMIN is not defined
#endif

#ifndef HUM_RMAX
    #error HUM_RMAX is not defined
#endif

static float tem_curr = 00.0f;
static float tem_xpcd = 37.0f;

static float hum_curr = 00.0f;
static float hum_xpcd = 45.0f;

float random_range(float min, float max)
{
    return (float)rand()/(float)(RAND_MAX/(max - min)) + min;
}

/// @brief Collect temperature value from sensor
void collect_tem()
{
    for (TickType_t d = pdMS_TO_TICKS(1000); ; vTaskDelay(d))
    {
        tem_curr = random_range(TEM_RMIN, TEM_RMAX);
        // if (tem_curr > tem_xpcd) ESP_LOGW("SENSOR", "Temperature is over limit (%f > %f)", tem_curr, tem_xpcd);
    }

    vTaskDelete(NULL);
}

float get_curr_tem()
{
    return tem_curr;
}

float get_xpcd_tem()
{
    return tem_xpcd;
}

void set_xpcd_tem(float expected_temperature)
{
    tem_xpcd = expected_temperature;
}

/// @brief Collect humidity value from sensor
void collect_hum()
{
    for (TickType_t d = pdMS_TO_TICKS(1000); ; vTaskDelay(d))
    {
        hum_curr = random_range(HUM_RMIN, HUM_RMAX);
        // if (hum_curr < hum_xpcd) ESP_LOGW("SENSOR", "Humidity is under limit (%f < %f)", hum_curr, hum_xpcd);
    }

    vTaskDelete(NULL);
}

float get_curr_hum()
{
    return hum_curr;
}

float get_xpcd_hum()
{
    return hum_xpcd;
}

void set_xpcd_hum(float expected_humidity)
{
    hum_xpcd = expected_humidity;
}

/// @brief Initialize sensor tasks
void init_sensor()
{
    xTaskCreate(collect_tem, "temperature", 8192, NULL, 5, NULL);
    xTaskCreate(collect_hum, "humidity", 8192, NULL, 5, NULL);
}
