/**
 * @file ds18b20.h
 * @brief ds18b20 driver based on simple software bitbang 
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../core/onewire.h"

/**
 * @brief ds18b20 sensor 
 * @param data_pin onewire bus data pin 
 * @param temperature_x10 last measured temperature in fixed point format e.g. 225 means 22.5°C
 * @param error_count number of failed measurements 
 * @param success_count number of successful measurements 
 * @param is_active true - last conversion was successful, false - otherwise
 * @param is_converting true - sensor is performing temperature conversion, false - conversion done or failed
 * @param conversion_started ticks timestamp for last conversion
 * @param scratchpad buffer for data received from DS18B20
 */
typedef struct
{
    gpio_pin_t data_pin;
    int16_t temperature_x10;
    uint32_t error_count;
    uint32_t success_count;
    bool is_active;
    bool is_converting;
    uint32_t conversion_started;
    uint8_t scratchpad[9];
} ds18b20_sensor_t;


/**
 * @brief initialize ds18b20 sensor 
 * @param sensor ds18b20 sensor pointer
 * @param data_pin onewire bus data pin 
 */
void ds18b20_init(ds18b20_sensor_t *sensor, gpio_pin_t data_pin);

/**
 * @brief start non-blocking DS18B20 temperature conversion or update temperature if conversion done
 * @param sensor ds18b20 sensor pointer
 */
void ds18b20_update(ds18b20_sensor_t *sensor);

#ifdef __cplusplus
}
#endif