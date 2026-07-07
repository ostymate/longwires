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
#define LED_OFF 0

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

#define CH0_SMUX_ADDR 0x00

#define SMUX_CONFIG_SIZE 21 /* [0] == CH0_SMUX_ADDR and [1..20] are smux bytes */

/* CH0 == VIOLET, CH1 == BLUE, CH2 == SKY, CH3 == CYAN, CH4 == FULL1, CH5 == NEAR_IR1 */
static const uint8_t f1f4_full_nir[] = {CH0_SMUX_ADDR, 0x30, 0x01, 0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x20, 0x04, 0x00, 0x30, 0x01, 0x50, 0x00, 0x06};

/* CH0 == GREEN, CH1 == YELLOW, CH2 == ORANGE, CH3 == RED, CH4 == FULL2, CH5 == NEAR_IR2 */
static const uint8_t f5f8_full_nir[] = {CH0_SMUX_ADDR, 0x00, 0x00, 0x00, 0x40, 0x02, 0x00, 0x10, 0x03, 0x50, 0x10, 0x03, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x50, 0x00, 0x06};

/* channels index */
#define VIOLET 0
#define BLUE 2
#define SKY 4
#define CYAN 6
#define FULL1 8
#define NEAR_IR1 10
#define GREEN 12
#define YELLOW 14
#define ORANGE 16
#define RED 18
#define FULL2 20
#define NEAR_IR2 22

#define CHANNEL(INDEX) ((uint32_t)(channels[(INDEX)]) | (uint32_t)(channels[(INDEX + 1)]) << 8)

#define READ_BYTES(_I2C, _REG, _BUF, _LEN)                     \
    do                                                         \
    {                                                          \
        if (!i2c_write(_I2C, ((uint8_t[]){(_REG)}), 1, false)) \
            return false;                                      \
        if (!i2c_read(_I2C, (_BUF), (_LEN), true))             \
            return false;                                      \
    } while (0)

#define WRITE_REG(_I2C, _REG, _VAL)                                   \
    do                                                                \
    {                                                                 \
        if (!i2c_write(_I2C, ((uint8_t[]){(_REG), (_VAL)}), 2, true)) \
            return false;                                             \
    } while (0)

#define WRITE_REG16(_I2C, _REG, _LSB, _MSB)                                   \
    do                                                                        \
    {                                                                         \
        if (!i2c_write(_I2C, ((uint8_t[]){(_REG), (_LSB), (_MSB)}), 3, true)) \
            return false;                                                     \
    } while (0)

static bool wait_bit_state(i2c_device_t *i2c, uint8_t reg, uint8_t bit, bool expected_state)
{
    uint8_t buf;
    uint32_t start, now;
    uint32_t timeout = US_TO_TICKS(BIT_WAIT_TIMEOUT_US);
    GET_TICK(start);
    do
    {
        GET_TICK(now);
        READ_BYTES(i2c, reg, &buf, 1);
        if (now - start > timeout)
            return false;
    } while (((bool)(buf & bit)) != expected_state);

    return true;
}

static bool setup_smux_and_measure(i2c_device_t *i2c, const uint8_t *smux_config)
{
    WRITE_REG(i2c, ENABLE, PON);
    WRITE_REG(i2c, SMUX_MODE, SMUX_WRITE);
    if (!i2c_write(i2c, smux_config, SMUX_CONFIG_SIZE, true))
        return false;
    WRITE_REG(i2c, ENABLE, PON | SMUX_EN);

    /* wait till device resets SMUX_EN bit */
    if (!wait_bit_state(i2c, ENABLE, SMUX_EN, false))
        return false;

    /* start spectral measurement */
    WRITE_REG(i2c, ENABLE, PON | SP_EN);

    /* wait till measurement ends*/
    return wait_bit_state(i2c, SPECTRAL_STATUS, SPECTRAL_VALID, true);
}

static bool set_led_brightness(i2c_device_t *i2c, uint8_t led_brightness)
{
    WRITE_REG(i2c, BANK_SEL, BANK1);
    WRITE_REG(i2c, LED_ON, ((led_brightness > LED_OFF) << LED_ON_BIT));
    WRITE_REG(i2c, LED_BRIGHTNESS, (LED_ACT | (led_brightness < MAX_BRIGHTNESS ? led_brightness : MAX_BRIGHTNESS)));
    WRITE_REG(i2c, BANK_SEL, BANK0);
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

    WRITE_REG(i2c, ENABLE, PON);
    WRITE_REG(i2c, ATIME, ATIME_DEFAULT);
    WRITE_REG16(i2c, ASTEP, ASTEP_LSB, ASTEP_MSB);
    WRITE_REG(i2c, AGAIN, AGAIN_1X);

    if (!set_led_brightness(i2c, led_brightness))
        return false;

    uint8_t channels[24];

    if (!setup_smux_and_measure(i2c, f1f4_full_nir))
        return false;
    READ_BYTES(i2c, DATA_START, channels, 12); /* read  f1:f4 + FULL1 + NEAR_IR1 (channels[0:11])*/

    if (!setup_smux_and_measure(i2c, f5f8_full_nir))
        return false;
    READ_BYTES(i2c, DATA_START, (channels + 12), 12); /* read f5:f8 + FULL2 + NEAR_IR2 (channels[12:23]) */

    set_led_brightness(i2c, LED_OFF);

    dev->full = (CHANNEL(FULL1) + CHANNEL(FULL2)) / 2UL;
    dev->near_ir = (CHANNEL(NEAR_IR1) + CHANNEL(NEAR_IR2)) / 2UL;

    dev->violet = CHANNEL(FULL1) ? ((CHANNEL(VIOLET) * CHANNEL(FULL2)) / CHANNEL(FULL1)) : CHANNEL(VIOLET);
    dev->blue = CHANNEL(FULL1) ? ((CHANNEL(BLUE) * CHANNEL(FULL2)) / CHANNEL(FULL1)) : CHANNEL(BLUE);
    dev->sky = CHANNEL(FULL1) ? ((CHANNEL(SKY) * CHANNEL(FULL2)) / CHANNEL(FULL1)) : CHANNEL(SKY);
    dev->cyan = CHANNEL(FULL1) ? ((CHANNEL(CYAN) * CHANNEL(FULL2)) / CHANNEL(FULL1)) : CHANNEL(CYAN);

    dev->green = CHANNEL(FULL2) ? ((CHANNEL(GREEN) * CHANNEL(FULL1)) / CHANNEL(FULL2)) : CHANNEL(GREEN);
    dev->yellow = CHANNEL(FULL2) ? ((CHANNEL(YELLOW) * CHANNEL(FULL1)) / CHANNEL(FULL2)) : CHANNEL(YELLOW);
    dev->orange = CHANNEL(FULL2) ? ((CHANNEL(ORANGE) * CHANNEL(FULL1)) / CHANNEL(FULL2)) : CHANNEL(ORANGE);
    dev->red = CHANNEL(FULL2) ? ((CHANNEL(RED) * CHANNEL(FULL1)) / CHANNEL(FULL2)) : CHANNEL(RED);

    return true;
}
