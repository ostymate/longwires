/**
 * @file sht3x.h
 * @brief software I2C bitbang driver for SHT3X
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "../core/i2c.h"

    typedef struct
    {
        int16_t temp_x10;        /** last measured temperature in fixed point format e.g. 225 means 22.5°C */
        uint8_t hum;             /** last measured humidity % */
        i2c_device_t i2c_device; /** I2C device structure */
    } sht3x_sensor_t;            /** SHT3X sensor structure */

    /**
     * @brief initialize SHT3X sensor
     *        set default values: temp_x10 = INT16_MAX; hum = UINT8_MAX;
     * @param sensor SHT3X sensor structure pointer
     * @param sda_pin I2C bus SDA pin
     * @param scl_pin I2C bus SCL pin
     * @param default_addr true for default I2C address (0x44), false for alternative address (0x45)
     * @return true for successful init, false otherwise
     */
    bool sht3x_init(sht3x_sensor_t *sensor, gpio_pin_t sda_pin, gpio_pin_t scl_pin, bool default_addr);

    /**
     * @brief perform SHT3X measurement.
     *        On success updates .temp_x10 and .hum.
     *        On failure does not.
     *        Same behavior for optional float values
     * @param sensor SHT3X sensor structure pointer
     * @param temperature send pointer if you need float value or NULL.
     * @param humidity send pointer if you need float value or NULL
     * @return true if measurement was successful, false otherwise
     */
    bool sht3x_read(sht3x_sensor_t *sensor, float *temperature, float *humidity);

#ifdef __cplusplus
}
#endif