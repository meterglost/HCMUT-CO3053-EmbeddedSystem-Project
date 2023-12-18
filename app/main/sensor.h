#ifndef __SENSOR_H__
#define __SENSOR_H__

#define TEM_RMIN 20.0f
#define TEM_RMAX 40.0f

#define HUM_RMIN 40.0f
#define HUM_RMAX 80.0f

void init_sensor();

/// @brief Get current collected temperature value
float get_curr_tem();

/// @brief Get temperature upper limit
float get_xpcd_tem();

/// @brief Get current collected humidity value
float get_curr_hum();

/// @brief Get humidity lower limit
float get_xpcd_hum();

/// @brief Set temperature upper limit to trigger action
/// @param expected_temperature
void set_xpcd_tem(float expected_temperature);

/// @brief Set humidity lower limit to trigger action
/// @param expected_humidity
void set_xpcd_hum(float expected_humidity);

#endif
