/**
 * @file as7341.c
 * @brief as7341 multispectral sensor driver implementation
 */

#include "as7341.h"

#define I2C_ADDR 0x39

#define ENABLE 0x80
#define PON (1 << 0)
#define SP_EN (1 << 1)
#define SMUX_EN (1 << 4)

#define BANK_SEL 0xA9
#define BANK1 (1 << 4)
#define BANK0 0

#define LED_ON 0x70
#define LED_ON_BIT 3

#define LED_BRIGHTNESS 0x74
#define LED_ACT (1 << 7)
#define MAX_BRIGHTNESS 0x7F
#define MIN_BRIGHTNESS 0

#define SPECTRAL_STATUS 0xA3
#define SPECTRAL_VALID (1 << 6)

#define SMUX_MODE 0xAF
#define SMUX_WRITE (1 << 4)

#define ATIME 0x81
#define ATIME_DEFAULT 98U /* (98 + 1) * (661 + 1) == 65538 (only 3 counts more than 0xFFFF) */

#define ASTEP 0xCA /* 0xCA == ASTEP_LSB, 0xCB == ASTEP_MSB */
#define ASTEP_LSB ((uint8_t)661U)
#define ASTEP_MSB ((uint8_t)(661U >> 8))

#define BIT_WAIT_TIMEOUT_US 1000000UL /* 1s timeout */

#define AGAIN 0xAA
#define AGAIN_1X 1

#define DATA_START 0x95 /* 0x95 == CH0_LSB, 0x96 == CH0_MSB, 0x97 == CH1_LSB ...*/

#define SMUX_CONFIG_SIZE 21 /* [0] byte is 0x00 addr and [1..20] are smux bytes */

/* CH0 == VIOLET, CH1 == BLUE, CH2 == SKY, CH3 == CYAN, CH4 == FULL1, CH5 == NEAR_IR1 */
static const uint8_t f1f4_full_nir[] = {0x00, 0x30, 0x01, 0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x20, 0x04, 0x00, 0x30, 0x01, 0x50, 0x00, 0x06};

/* CH0 == GREEN, CH1 == YELLOW, CH2 == ORANGE, CH3 == RED, CH4 == FULL2, CH5 == NEAR_IR2 */
static const uint8_t f5f8_full_nir[] = {0x00, 0x00, 0x00, 0x00, 0x40, 0x02, 0x00, 0x10, 0x03, 0x50, 0x10, 0x03, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x50, 0x00, 0x06};

/* channels index */
#define VIOLET 0
#define BLUE 1
#define SKY 2
#define CYAN 3
#define FULL1 4
#define NEAR_IR1 5
#define GREEN 6
#define YELLOW 7
#define ORANGE 8
#define RED 9
#define FULL2 10
#define NEAR_IR2 11

static bool wait_bit_state(i2c_device_t *i2c, uint8_t reg, uint8_t bit, bool expected_state)
{
    uint8_t val;
    uint32_t start, now;
    uint32_t timeout = US_TO_TICKS(BIT_WAIT_TIMEOUT_US);
    GET_TICK(start);
    do
    {
        GET_TICK(now);
        if (!i2c_write(i2c, (uint8_t[]){reg}, 1, false))
            return false;
        if (!i2c_read(i2c, &val, 1, true))
            return false;
        if (now - start > timeout)
            return false;
    } while (((bool)(val & bit)) != expected_state);

    return true;
}

static bool setup_smux_and_measure(i2c_device_t *i2c, const uint8_t *smux_config)
{
    if (!i2c_write(i2c, (uint8_t[]){ENABLE, PON}, 2, true))
        return false;
    if (!i2c_write(i2c, (uint8_t[]){SMUX_MODE, SMUX_WRITE}, 2, true))
        return false;
    if (!i2c_write(i2c, smux_config, SMUX_CONFIG_SIZE, true))
        return false;
    if (!i2c_write(i2c, (uint8_t[]){ENABLE, PON | SMUX_EN}, 2, true))
        return false;

    /* wait till device resets SMUX_EN bit */
    if (!wait_bit_state(i2c, ENABLE, SMUX_EN, false))
        return false;

    /* start spectral measurement */
    if (!i2c_write(i2c, (uint8_t[]){ENABLE, PON | SP_EN}, 2, true))
        return false;

    /* wait till measurement ends*/
    return wait_bit_state(i2c, SPECTRAL_STATUS, SPECTRAL_VALID, true);
}

static bool set_led_brightness(i2c_device_t *i2c, uint8_t led_brightness)
{
    if (!i2c_write(i2c, (uint8_t[]){BANK_SEL, BANK1}, 2, true))
        return false;
    if (!i2c_write(i2c, (uint8_t[]){LED_ON, ((led_brightness > 0) << LED_ON_BIT)}, 2, true))
        return false;
    if (!i2c_write(i2c, (uint8_t[]){LED_BRIGHTNESS, (LED_ACT | (led_brightness < MAX_BRIGHTNESS ? led_brightness : MAX_BRIGHTNESS))}, 2, true))
        return false;
    if (!i2c_write(i2c, (uint8_t[]){BANK_SEL, BANK0}, 2, true))
        return false;
    return true;
}

bool as7341_init(as7341_t *dev, gpio_pin_t sda, gpio_pin_t scl)
{
    if (!dev)
        return false;
    *dev = (as7341_t){0};
    return i2c_init(&dev->i2c_dev, sda, scl, I2C_ADDR);
}

bool as7341_read_all(as7341_t *dev, uint8_t led_brightness)
{
    if (!dev)
        return false;
    i2c_device_t *i2c = &dev->i2c_dev;

    if (!i2c_write(i2c, (uint8_t[]){ENABLE, PON}, 2, true))
        return false;
    if (!i2c_write(i2c, (uint8_t[]){ATIME, ATIME_DEFAULT}, 2, true))
        return false;
    if (!i2c_write(i2c, (uint8_t[]){ASTEP, ASTEP_LSB, ASTEP_MSB}, 3, true))
        return false;
    if (!i2c_write(i2c, (uint8_t[]){AGAIN, AGAIN_1X}, 2, true))
        return false;

    if (!set_led_brightness(i2c, led_brightness))
        return false;

    uint16_t channels[12];

    if (!setup_smux_and_measure(i2c, f1f4_full_nir))
        return false;
    if (!i2c_write(i2c, (uint8_t[]){DATA_START}, 1, false))
        return false;
    if (!i2c_read(i2c, (uint8_t *)channels, 12, true)) /* read  f1:f4 + FULL1 + NEAR_IR1 (channels[0:5])*/
        return false;

    if (!setup_smux_and_measure(i2c, f5f8_full_nir))
        return false;
    if (!i2c_write(i2c, (uint8_t[]){DATA_START}, 1, false))
        return false;
    if (!i2c_read(i2c, (uint8_t *)(channels + 6), 12, true)) /* read f5:f8 + FULL2 + NEAR_IR2 (channels[6:11]) */
        return false;

    set_led_brightness(i2c, MIN_BRIGHTNESS);

    dev->full = ((uint32_t)(channels[FULL1]) + (uint32_t)(channels[FULL2])) / 2UL;

    if (!(channels[FULL1]))
        channels[FULL1] = 1;

    if (!(channels[FULL2]))
        channels[FULL2] = 1;

    dev->violet = ((uint32_t)(channels[VIOLET]) * (uint32_t)(channels[FULL2])) / (uint32_t)(channels[FULL1]);
    dev->blue = ((uint32_t)(channels[BLUE]) * (uint32_t)(channels[FULL2])) / (uint32_t)(channels[FULL1]);
    dev->sky = ((uint32_t)(channels[SKY]) * (uint32_t)(channels[FULL2])) / (uint32_t)(channels[FULL1]);
    dev->cyan = ((uint32_t)(channels[CYAN]) * (uint32_t)(channels[FULL2])) / (uint32_t)(channels[FULL1]);

    dev->green = ((uint32_t)(channels[GREEN]) * (uint32_t)(channels[FULL1])) / (uint32_t)(channels[FULL2]);
    dev->yellow = ((uint32_t)(channels[YELLOW]) * (uint32_t)(channels[FULL1])) / (uint32_t)(channels[FULL2]);
    dev->orange = ((uint32_t)(channels[ORANGE]) * (uint32_t)(channels[FULL1])) / (uint32_t)(channels[FULL2]);
    dev->red = ((uint32_t)(channels[RED]) * (uint32_t)(channels[FULL1])) / (uint32_t)(channels[FULL2]);

    dev->near_ir = ((uint32_t)(channels[NEAR_IR1]) + (uint32_t)(channels[NEAR_IR2])) / 2UL;

    return true;
}
