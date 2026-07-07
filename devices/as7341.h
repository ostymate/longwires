/**
 * @file as7341.h
 * @brief as7341 multispectral sensor driver
 * 
 */


#ifdef __cplusplus
extern "C"
{
#endif

#include "../core/i2c.h"

    typedef struct
    {
        uint16_t violet;      /** F1 415 nm */
        uint16_t blue;        /** F2 445 nm */
        uint16_t sky;         /** F3 480 nm */
        uint16_t cyan;        /** F4 515 nm */
        uint16_t green;       /** F5 555 nm */
        uint16_t yellow;      /** F6 590 nm */
        uint16_t orange;      /** F7 630 nm */
        uint16_t red;         /** F8 680 nm */
        uint16_t near_ir;     /** NIR 910 nm */
        uint16_t full;        /** clear non filtered */
        i2c_device_t i2c_dev; /** i2c device structure */
    } as7341_t;               /** as7341 device structure */

    /**
     * @brief initialize as7341 sensor
     * @param dev as7341_t device structure pointer
     * @param sda sda pin
     * @param scl scl pin
     * @return true for successful init, false otherwise
     * @note after init and before first successful measurement all channels == 0
     */
    bool as7341_init(as7341_t *dev, gpio_pin_t sda, gpio_pin_t scl);

    /**
     * @brief read all channels
     * @param dev device structure pointer
     * @param led_brightness 0...127, 0 == led off, 127 and above == MAX brightness
     * @return true - successful measurement, false otherwise
     * @note updates spectral data only after successful measurement, otherwise doesn't modify 'em
     * @note the whole uint16_t range (0...65535) are valid values for all channels
     * @note 65535 means saturation 
     */
    bool as7341_read_all(as7341_t *dev, uint8_t led_brightness);

#ifdef __cplusplus
}
#endif
