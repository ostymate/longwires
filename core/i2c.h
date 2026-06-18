/**
 * @file i2c.h
 * @brief adaptive software bitbang I2C implementation
 * @warning It's blocking! Be careful when use it with slow devices or long wires, as it may block until timeout occurs.

 * Features:
 * Software I2C master mode
 * Bitbang only. no hardware I2C support needed
 * Adaptive I2C speed depending on line params
 * Clock stretching for all I2C operations
 * Optional repeated start - no stop condition between operations if needed
 * Simple API: init, write, read
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "platforms_common.h"

/**
 * @brief I2C device structure
 * @param sda SDA pin (any pin that supports open drain configuration)
 * @param scl SCL pin (the same requirements as for SDA pin)
 * @param addr I2C device address (7-bit)
 * @param t_hold_ticks adaptive timing based on line measurement. do not modify
 * @param t_timeout_ticks SDA/SCL rise timeout ticks
 */
typedef struct i2c_device_t
{
    gpio_pin_t sda;
    gpio_pin_t scl;
    uint8_t addr;
    uint32_t t_hold_ticks;
    uint32_t timeout_ticks;
} i2c_device_t;

/**
 * @brief I2C device initialization. Call it once before use (although, recall isn't dangerous)
 * @param dev I2C device structure pointer
 * @param sda SDA pin (any pin that supports open drain configuration)
 * @param scl SCL pin (the same requirements as for SDA pin)
 * @param addr I2C device address (7-bit)
 * @return true successful init, false otherwise
 */
bool i2c_init(i2c_device_t *dev, gpio_pin_t sda, gpio_pin_t scl, uint8_t addr);

/**
 * @brief write data to device
 * @param dev I2C device structure pointer
 * @param data data to write
 * @param len size of data
 * @param stop true: perform stop condition after writing || false: do NOT perform (false == repeated start mode)
 * @return true: slave received all bytes with ACK || false: NACK or timeout
 */
bool i2c_write(i2c_device_t *dev, const uint8_t *data, uint32_t len, bool stop);

/**
 * @brief read data from device
 * @param dev I2C device structure pointer
 * @param buffer data buffer
 * @param len size of data to read
 * @param stop true: perform stop condition after reading || false: do NOT perform (false == repeated start mode)
 * @return true: all bytes were read successfully || false: timeout
 */
bool i2c_read(i2c_device_t *dev, uint8_t *buffer, uint32_t len, bool stop);


#ifdef __cplusplus
}
#endif