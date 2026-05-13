/**
 * @file bb_i2c.h
 * @brief universal and simple software I2C implementation
 * @warning It's blocking! Be careful with clock stretching timeout and low frequency
 *
 * Features:
 * Software I2C master mode 
 * Bitbang only. no hardware I2C support needed
 * Wide range I2C frequency (1Hz <> 1MHz @ 240MHz ESP32 tested)
 * Clock stretching for all I2C operations
 * Repeated start - no stop condition between operations
 * Available MCUs: ESP32 (XTensa and RISC-V series), STM32F1
 *
 **/

#pragma once

#include "bb_utils.h"

/** Default I2C frequency: 100KHz */
#define I2C_DEFAULT_FREQ_HZ 100000UL

/** Default clock stretching timeout: 25ms   */
#define DEFAULT_CLOCK_STRETCHING_TIMEOUT_US 25000UL

/**
 * @brief I2C device structure
 * @param sda_pin SDA pin (any pin that supports open drain input-output mode)
 * @param scl_pin SCL pin (Also needs OD with input-output)
 * @param addr I2C device address
 * @param freq_hz I2C device frequency
 */
typedef struct i2c_bb_device_t
{
    gpio_pin_t sda_pin;
    gpio_pin_t scl_pin;
    uint8_t addr;
    uint32_t freq_hz;
} i2c_bb_device_t;

/**
 * @brief I2C device initialization. Call it once before use (although, recall isn't dangerous)
 * @param i2c_bb_device I2C device structure
 */
void i2c_bb_init(i2c_bb_device_t *i2c_bb_device);

/**
 * @brief complete I2C transaction: write data and immediately read
 * @param i2c_bb_device I2C device structure
 * @param data bytes to write
 * @param data_len size of data
 * @param buffer buffer for bytes to read
 * @param buffer_len size of buffer
 * @param write_stop true: send stop condition after writing || false: do NOT send stop condition after writing (repeated start between writing and reading)
 * @param read_stop true: send stop condition after reading || false: do NOT send stop condition after reading (repeated start for next operation)
 * @return true: transaction was completely successful || false: we got some troubles
 */
bool i2c_bb_transaction(i2c_bb_device_t *i2c_bb_device,
                        const uint8_t *data, uint32_t data_len, uint8_t *buffer,
                        uint32_t buffer_len, bool write_stop, bool read_stop);

/**
 * @brief write data to device
 * @param i2c_bb_device I2C device structure
 * @param data data to write
 * @param len size of data
 * @param write_stop true: send stop condition after writing || false: do NOT send stop condition after writing (false = repeated start mode)
 * @return true: slave received all bytes with ACK || false: maybe NACK or CS timeout or whatever
 */
bool i2c_bb_write(i2c_bb_device_t *i2c_bb_device, const uint8_t *data, uint32_t len, bool write_stop);

/**
 * @brief read data from device
 * @param i2c_bb_device I2C device structure
 * @param buffer data buffer
 * @param len size of data to read
 * @param read_stop true: send stop conditon after reading || false: do NOT send stop condition after reading (false = repeated start mode)
 * @return true: all bytes were read successfully || false: CS timeout or another issue
 */
bool i2c_bb_read(i2c_bb_device_t *i2c_bb_device, uint8_t *buffer, uint32_t len, bool read_stop);

/**
 * @brief ping device (send I2C write command without data)
 * @param i2c_bb_device I2C device structure
 * @return true: device available and sent ACK || false: NACK or CS timeout
 * @warning some devices may NOT support it
 */
bool i2c_bb_device_available(i2c_bb_device_t *i2c_bb_device);
