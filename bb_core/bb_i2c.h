/**
 * @file bb_i2c.h
 * @brief adaptive software bitbang I2C implementation with clock stretching support
 * @warning It's blocking! Be careful if set to low frequency
 * 
 * Features:
 * Software I2C master mode 
 * Bitbang only. no hardware I2C support needed
 * Adaptive I2C speed depending on line params
 * Wide frequency range: from very low (for long wires and slow devices) to standard 100kHz and even up to 1MHz (if wires are short and devices are fast)
 * Clock stretching for all I2C operations
 * Optional repeated start - no stop condition between operations if needed
 * Simple API: init, write, read, transaction (write followed by read with repeated start in between), device availability check (ping)
 * Robust level setting with confirmation (for stable timing even with long wires and slow devices)
 *
 */

#pragma once

#include "bb_utils.h"

/**
 * @brief I2C device structure
 * @param sda_pin SDA pin (any pin that supports open drain input-output mode)
 * @param scl_pin SCL pin (Also needs OD with input-output)
 * @param addr I2C device address
 * @param freq_hz user defined I2C device frequency
 * @param actual_freq_hz actual I2C frequency based on adaptive timing setup 
 * @param t_hold_ticks adaptive delay ticks for half clock period 
 * @param t_rise_sda_ticks measured rise time for SDA line in ticks 
 * @param t_rise_scl_ticks measured rise time for SCL line in ticks 
 * @warning if freq_hz is set to 0, default frequency will be used (100kHz)
 */
typedef struct i2c_bb_device_t
{
    gpio_pin_t sda_pin;
    gpio_pin_t scl_pin;
    uint8_t addr;
    uint32_t freq_hz;
    uint32_t actual_freq_hz;
    uint32_t t_hold_ticks;
    uint32_t t_rise_sda_ticks;
    uint32_t t_rise_scl_ticks;
} i2c_bb_device_t;

/**
 * @brief I2C device initialization. Call it once before use (although, recall isn't dangerous)
 * @param device I2C device structure
 */
void i2c_bb_init(i2c_bb_device_t *device);

/**
 * @brief complete I2C transaction: write data and immediately read
 * @param device I2C device structure
 * @param data bytes to write
 * @param data_len size of data
 * @param buffer buffer for bytes to read
 * @param buffer_len size of buffer
 * @param write_stop true: send stop condition after writing || false: do NOT send stop condition after writing (repeated start between writing and reading)
 * @param read_stop true: send stop condition after reading || false: do NOT send stop condition after reading (repeated start for next operation)
 * @return true: transaction was completely successful || false: we got some troubles
 */
bool i2c_bb_transaction(i2c_bb_device_t *device,
                        const uint8_t *data, uint32_t data_len, uint8_t *buffer,
                        uint32_t buffer_len, bool write_stop, bool read_stop);

/**
 * @brief write data to device
 * @param device I2C device structure
 * @param data data to write
 * @param len size of data
 * @param write_stop true: send stop condition after writing || false: do NOT send stop condition after writing (false = repeated start mode)
 * @return true: slave received all bytes with ACK || false: maybe NACK or CS timeout or whatever
 */
bool i2c_bb_write(i2c_bb_device_t *device, const uint8_t *data, uint32_t len, bool write_stop);

/**
 * @brief read data from device
 * @param device I2C device structure
 * @param buffer data buffer
 * @param len size of data to read
 * @param read_stop true: send stop conditon after reading || false: do NOT send stop condition after reading (false = repeated start mode)
 * @return true: all bytes were read successfully || false: CS timeout or another issue
 */
bool i2c_bb_read(i2c_bb_device_t *device, uint8_t *buffer, uint32_t len, bool read_stop);

/**
 * @brief ping device (send I2C write command without data)
 * @param device I2C device structure
 * @return true: device available and sent ACK || false: NACK or CS timeout
 * @warning some devices may NOT support it
 */
bool i2c_bb_device_available(i2c_bb_device_t *device);
