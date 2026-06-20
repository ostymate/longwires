/**
 * @file ds18b20.h
 * @brief non-blocking ds18b20 driver based on simple software bitbang 
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../core/onewire.h"

#define DS18B20_INIT_VALUE 850
#define DS18B20_TEMP_NEVER_UPDATED UINT32_MAX

/**
 * @brief ds18b20 sensor struct
 * @param data_pin onewire data pin 
 * @param temp_x10 last measured temperature in fixed point format e.g. 225 means 22.5°C. being updated only after valid conversion
 * @param last_valid_conversion timestamp (ticks) when last valid conversion was received and temperature was updated 
 * @param conversion_started timestamp (ticks) when last conversion was started
 */
typedef struct
{
    gpio_pin_t data_pin;
    int16_t temp_x10;
    uint32_t last_valid_conversion;
    uint32_t conversion_started;
} ds18b20_sensor_t;


/**
 * @brief initialize ds18b20 sensor and start first conversion
 * @note after init (no matter successful or not) and before first valid conversion .temp_x10 == DS18B20_INIT_VALUE (85.0`C) 
 * @param sensor ds18b20 sensor struct pointer
 * @param data_pin onewire bus data pin 
 * @return true sensor initialized (presence detected, first conversion started), false init failed (no presence)
 */
bool ds18b20_init(ds18b20_sensor_t *sensor, gpio_pin_t data_pin);

/**
 * @brief start non-blocking DS18B20 temperature conversion or update temperature if conversion done
 * @note .temp_x10 being updated only after successful conversion, after failed one it remains unchanged
 *       .temp_x10 == DS18B20_INIT_VALUE before at least one conversion was properly done.
 *       DS18B20 sensor can't measure exactly 85.0`C. because there are no way to distinguish valid 85.0`C from the initial sensor value 
 * @param sensor ds18b20 sensor struct pointer
 * @param temp optional pointer if temperature in float needed or NULL. 
 * @return last valid temperature "age" in microseconds. if there is no valid data yet return DS18B20_TEMP_NEVER_UPDATED
 */
uint32_t ds18b20_update(ds18b20_sensor_t *sensor, float *temp);

#ifdef __cplusplus
}
#endif