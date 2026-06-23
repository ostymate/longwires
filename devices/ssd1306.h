/**
 * @file ssd1306.h
 * @brief SSD1306 OLED display driver for ASCII text
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
#define BUF_SIZE (1 + OLED_WIDTH) /* first byte is DATA_BYTE, followed by page content */

    typedef struct
    {
        uint8_t buf[BUF_SIZE];   /** buffer for page data and commands */
        i2c_device_t i2c_device; /** I2C device structure */
    } oled_t;                    /** OLED device structure */

    /**
     * @brief Initialize the OLED display
     * @param dev Pointer to the OLED device structure
     * @param sda_pin GPIO pin for I2C SDA
     * @param scl_pin GPIO pin for I2C SCL
     * @param use_default_addr true: use default I2C address (0x3C), false: use alternate address (0x3D)
     * @param flip true to flip the display 180 degrees, false for normal orientation
     * @return true on success, false on failure
     */
    bool oled_init(oled_t *dev, gpio_pin_t sda_pin, gpio_pin_t scl_pin, bool use_default_addr, bool flip);

    /**
     * @brief Print ASCII text
     * @note Autowraps at screen edge, truncates at screen bottom,
     *       `\n` forces newline, empty string clears display.
     *       Only printable ASCII characters 0x20–0x7E are printed; others terminate the string.
     * @param dev Pointer to the OLED device structure
     * @param str String to print
     * @return true on success, false on failure
     */
    bool oled_print(oled_t *dev, const char *str);

#ifdef __cplusplus
}
#endif