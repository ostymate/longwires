/**
 * @file ds3231.h
 * @brief ds3231 RTC module driver
 */
#pragma once 

#ifdef __cplusplus
extern "C" {
#endif

#include "../core/i2c.h"



/**
 * @brief DS3231 RTC device struct
 * @param second 0-59 
 * @param minute 0-59
 * @param hour 0-23 
 * @param day 1-31
 * @param month 1-12
 * @param year 2000 - 2100
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
    i2c_device_t i2c_device;
} ds3231_t;

/**
 * @brief initialize ds3231 rtc device
 * @param ds3231 RTC device 
 * @param sda_pin I2C bus SDA pin
 * @param scl_pin I2C bus SCL pin 
 * @return true init was successful 
 */
bool ds3231_init(ds3231_t *ds3231, gpio_pin_t sda_pin, gpio_pin_t scl_pin);

/**
 * @brief get time and date from rtc 
 * @note on success all time fields are updated, on failure remain unchanged 
 * @param ds3231 RTC device
 * @return true time is valid and received successfully 
 */
bool ds3231_get_time(ds3231_t *ds3231);

/**
 * @brief set time and date in DS3231 RTC module 
 * @param ds3231 RTC device
 * @return true time is valid and set successfully 
 */
bool ds3231_set_time(ds3231_t *ds3231);

#ifdef __cplusplus
}
#endif