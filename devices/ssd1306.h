/**
 * @file ssd1306.h
 * @brief SSD1306 OLED display driver header for teletype and page text mode
 * 
 */
#pragma once
#ifdef __cplusplus
extern "C"
{
#endif

#include "../core/i2c.h"

#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define BUF_SIZE (1 + OLED_WIDTH) // DATA_BYTE + page content

    typedef struct
    {
        uint8_t scroll_offset;
        uint8_t buf[BUF_SIZE];
        i2c_device_t i2c_device;
    } oled_t;

    bool oled_init(oled_t *dev, gpio_pin_t sda_pin, gpio_pin_t scl_pin, bool use_default_addr, bool flip);
    bool oled_print_page(oled_t *dev, const char *str, uint8_t page);
    void oled_print(oled_t *dev, const char *str);

#ifdef __cplusplus
}
#endif