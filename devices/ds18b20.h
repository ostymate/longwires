/**
 * @file ds18b20.h
 * @brief non-blocking ds18b20 driver based on simple software bitbang
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "../core/onewire.h"

#define DS18B20_TEMP_NOT_MEASURED INT16_MIN

    typedef struct
    {
        gpio_pin_t data_pin;         /* onewire data pin  */
        int16_t temp_x10;            /* last measured temperature in fixed point format e.g. 225 means 22.5°C. being updated only after valid conversion.  */
        uint32_t last_temp_update;   /* timestamp (ticks) when last valid conversion was received and temperature was updated */
        uint32_t conversion_started; /* timestamp (ticks) when last conversion was started */
    } ds18b20_sensor_t;              /* ds18b20 sensor struct */

    /**
     * @brief initialize ds18b20 sensor and start first conversion
     * @note after init (no matter successful or not) and before first valid conversion .temp_x10 == DS18B20_TEMP_NOT_MEASURED (INT16_MIN)
     * @param sensor ds18b20 sensor struct pointer
     * @param data_pin onewire bus data pin
     * @return true sensor initialized (presence detected, first conversion started), false init failed (no presence)
     */
    bool ds18b20_init(ds18b20_sensor_t *sensor, gpio_pin_t data_pin);

    /**
     * @brief start non-blocking DS18B20 temperature conversion or update temperature if conversion done
     * @note .temp_x10 being updated only after successful conversion, after failed one it remain unchanged
     *       .temp_x10 == DS18B20_TEMP_NOT_MEASURED (INT16_MIN) before at least one conversion was properly done.
     * @param sensor ds18b20 sensor struct pointer
     * @param temp optional pointer if temperature in float needed or NULL. being updated only if there is at least one succesful conversion
     * @return true valid temperature data is available and a conversion is in progress. false otherwise
     */
    bool ds18b20_update(ds18b20_sensor_t *sensor, float *temp);

#ifdef __cplusplus
}
#endif