/**
 * @file sht3x.h
 * @brief software I2C bitbang driver for SHT3X
 */

#pragma once

#include "../bb_core/bb_i2c.h"

// Default values
#define SHT3X_DEFAULT_ADDR 0x44
#define SHT3X_ALT_ADDR 0x45
#define SHT3X_DEFAULT_FREQ 100000

/**
 * @brief SHT3X sensor
 * @param temp_x10 last measured temperature in fixed point format e.g. 225 means 22.5°C
 * @param hum last measured humidity %
 * @param is_active true if last measurement was successful, false - otherwise
 * @param error_count number of failed measurements
 * @param success_count number of sucessful measurements
 * @param i2c_bb_device I2C device structure
 */
typedef struct
{
    int16_t temp_x10;
    uint8_t hum;
    bool is_active;
    uint32_t error_count;
    uint32_t success_count;
    i2c_bb_device_t i2c_bb_device;
} sht3x_sensor_t;

/**
 * @brief initialize SHT3X sensor
 * @param sensor SHT3X sensor
 * @param sda_pin I2C bus SDA pin
 * @param scl_pin I2C bus SCL pin
 * @param default_addr true if using default I2C address (0x44), false if using alternative address (0x45)
 */
void sht3x_init(sht3x_sensor_t *sensor, gpio_pin_t sda_pin, gpio_pin_t scl_pin, bool default_addr);

/**
 * @brief start SHT3X measurement
 * @param sensor SHT3X sensor
 * @param temperature temperature pointer if temperature in float needed
 * @param humidity humidity pointer if humidity in float needed
 * @return true if measurement was successful, false - otherwise
 */
bool sht3x_read(sht3x_sensor_t *sensor, float *temperature, float *humidity);
