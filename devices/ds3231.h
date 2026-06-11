/**
 * @file ds3231.h
 * @brief ds3231 RTC module driver
 */
#pragma once 

#ifdef __cplusplus
extern "C" {
#endif

#include "../core/i2c.h"

#define DS3231_DEFAULT_ADDR 0x68

/**
 * @brief DS3231 RTC device
 * @param second 0-59 
 * @param minute 0-59
 * @param hour 0-23 
 * @param day 1-31
 * @param month 1-12
 * @param year 2000 - 2100
 * @param is_time_valid true if last updating or receiving time was successful, false otherwise 
 * @param i2c_device i2c device struct
 * 
 */
typedef struct
{
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
    bool is_time_valid;
    i2c_device_t i2c_device;
} ds3231_t;

/**
 * @brief initialize ds3231 rtc device
 * @param ds3231 RTC device 
 * @param sda_pin I2C bus SDA pin
 * @param scl_pin I2C bus SCL pin 
 */
void ds3231_init(ds3231_t *ds3231, gpio_pin_t sda_pin, gpio_pin_t scl_pin);

/**
 * @brief get time and date from rtc 
 * @param ds3231 RTC device
 */
void ds3231_get_time(ds3231_t *ds3231);

/**
 * @brief set time and date in DS3231 RTC module 
 * @param ds3231 RTC device
 */
void ds3231_set_time(ds3231_t *ds3231);

#ifdef __cplusplus
}
#endif