/**
 * @file onewire.h
 * @brief software bitbang implementation of onewire
 */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "platforms_common.h"

    /**
     * @brief check presence of the one wire device
     * @param data_pin onewire bus data pin
     * @return true - device send presence, false - no device presence
     */
    bool onewire_check_presence(gpio_pin_t data_pin);

    /**
     * @brief write byte of data
     * @param data byte to write
     */
    void onewire_write_byte(gpio_pin_t data_pin, uint8_t data);

    /**
     * @brief read one byte of data
     * @param data_pin onewire bus data pin
     * @return byte of data read from bus
     */
    uint8_t onewire_read_byte(gpio_pin_t data_pin);

    /**
     * @brief calculate crc-8 checksum
     * @param data pointer to data bytes
     * @param len data size
     * @return crc-8 checksum byte
     */
    uint8_t onewire_crc8(const uint8_t *data, uint32_t len);

#ifdef __cplusplus
}
#endif