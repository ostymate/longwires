/**
 * @file bb_i2c.c
 * @brief Bit-banged I2C. Works on 100m wires. Tolerates noise. Adaptive timings implemented.
 * @note "Frequency" depends on rise time of the signals, but not faster than 100kHz.
 *
 */

#include "bb_i2c.h"

#define I2C_BB_RISE_TIMEOUT_US 25000
#define I2C_BB_MIN_HOLD_US 5
#define HIGH_LEVEL_CONFIRMATION_SAMPLES_COUNT 5
#define CLEAN_BUS_SCL_CYCLES 9

static bool detect_high(gpio_pin_t pin, uint32_t timeout_ticks)
{
    uint32_t start_tick, current_tick;

    BB_GET_TICKS(start_tick);
    BB_GPIO_PIN_SET(pin);
    while (1)
    {
        BB_GET_TICKS(current_tick);

        if (BB_GPIO_PIN_READ(pin))
            return true;

        if ((current_tick - start_tick) >= timeout_ticks)
            return false;
    }
}

static void i2c_bb_start(i2c_bb_device_t *dev)
{
    detect_high(dev->scl, dev->timeout_ticks);
    detect_high(dev->sda, dev->timeout_ticks);    
    BB_DELAY_TICKS(dev->t_hold_ticks);
    BB_GPIO_PIN_RESET(dev->sda);
    BB_DELAY_TICKS(dev->t_hold_ticks);
    BB_GPIO_PIN_RESET(dev->scl);
    BB_DELAY_TICKS(dev->t_hold_ticks);
}

static void i2c_bb_stop(i2c_bb_device_t *dev)
{
    BB_GPIO_PIN_RESET(dev->sda);
    BB_DELAY_TICKS(dev->t_hold_ticks);
    BB_GPIO_PIN_SET(dev->scl);
    BB_DELAY_TICKS(dev->t_hold_ticks);
    BB_GPIO_PIN_SET(dev->sda);
    BB_DELAY_TICKS(dev->t_hold_ticks);
}

static void clean_bus(i2c_bb_device_t *dev)
{
    BB_GPIO_PIN_SET(dev->sda);

    for (uint32_t i = 0; i < CLEAN_BUS_SCL_CYCLES; i++)
    {
        /* set SCL low */
        BB_GPIO_PIN_RESET(dev->scl);
        BB_DELAY_TICKS(dev->t_hold_ticks);

        /* release SCL and process possible clock stretch waiting SCL HIGH untill timeout */
        BB_GPIO_PIN_SET(dev->scl);
        detect_high(dev->scl, dev->timeout_ticks);
        BB_DELAY_TICKS(dev->t_hold_ticks);
    }

    i2c_bb_stop(dev);
}

static uint32_t measure_t_hold_adaptive(gpio_pin_t pin, uint32_t timeout_ticks, uint32_t t_hold_ticks)
{
    uint32_t start_tick, current_tick, detected_tick = 0, samples = 0;

    BB_GET_TICKS(start_tick);
    BB_GPIO_PIN_SET(pin);
    while (1)
    {
        BB_GET_TICKS(current_tick);

        samples = BB_GPIO_PIN_READ(pin) ? samples + 1 : 0;
        detected_tick = samples > 1 ? detected_tick : current_tick;

        if (((samples >= HIGH_LEVEL_CONFIRMATION_SAMPLES_COUNT) && ((current_tick - detected_tick) >= t_hold_ticks)) || (current_tick - start_tick) >= timeout_ticks)
            return current_tick - start_tick;
    }
}

static void setup_adaptive_timing(i2c_bb_device_t *dev)
{
    dev->timeout_ticks = BB_US_TO_TICKS(I2C_BB_RISE_TIMEOUT_US);
    uint32_t min_hold_ticks = BB_US_TO_TICKS(I2C_BB_MIN_HOLD_US);
    dev->t_hold_ticks = min_hold_ticks;

    clean_bus(dev);

    BB_GPIO_PIN_RESET(dev->scl);
    BB_DELAY_TICKS(dev->t_hold_ticks);
    BB_GPIO_PIN_RESET(dev->sda);
    BB_DELAY_TICKS(dev->t_hold_ticks);

    /* Release SDA, measure rise */
    uint32_t sda_hold_adaptive = measure_t_hold_adaptive(dev->sda, dev->timeout_ticks, dev->t_hold_ticks);
    /* Release SCL, measure rise */
    uint32_t scl_hold_adaptive = measure_t_hold_adaptive(dev->scl, dev->timeout_ticks, dev->t_hold_ticks);

    dev->t_hold_ticks = (sda_hold_adaptive > scl_hold_adaptive ? sda_hold_adaptive : scl_hold_adaptive);

    clean_bus(dev);
}

static bool i2c_bb_write_byte(i2c_bb_device_t *dev, uint8_t byte)
{
    for (uint32_t bit = 8; bit--;)
    {
        /* set current bit */
        (byte >> bit) & 1 ? BB_GPIO_PIN_SET(dev->sda) : BB_GPIO_PIN_RESET(dev->sda);
        BB_DELAY_TICKS(dev->t_hold_ticks);

        /* release SCL and wait HIGH processing clock stretch*/
        if (!detect_high(dev->scl, dev->timeout_ticks))
            return false;
        BB_DELAY_TICKS(dev->t_hold_ticks);

        /*SCL LOW for next bit*/
        BB_GPIO_PIN_RESET(dev->scl);
        BB_DELAY_TICKS(dev->t_hold_ticks);
    }

    /* release SDA for ACK / NACK */
    BB_GPIO_PIN_SET(dev->sda);
    BB_DELAY_TICKS(dev->t_hold_ticks);

    /* process clock stretch before ACK / NACK */
    if (!detect_high(dev->scl, dev->timeout_ticks))
        return false;

    /* HIGH == NACK */
    if (detect_high(dev->sda, dev->t_hold_ticks))
        return false;

    /* SCL LOW for next byte */
    BB_GPIO_PIN_RESET(dev->scl);
    BB_DELAY_TICKS(dev->t_hold_ticks);

    return true;
}

static bool i2c_bb_read_byte(i2c_bb_device_t *dev, uint8_t *buf, bool ack)
{
    *buf = 0;

    /* release SDA before reading */ 
    BB_GPIO_PIN_SET(dev->sda);
    BB_DELAY_TICKS(dev->t_hold_ticks);
    
    for (uint32_t bit = 8; bit--;)
    {
        /* process clock stretch */
        if (!detect_high(dev->scl, dev->timeout_ticks))
            return false;

        /* read current bit */
        *buf |= (detect_high(dev->sda, dev->t_hold_ticks) << bit);
        BB_DELAY_TICKS(dev->t_hold_ticks);
        
        /* set SCL LOW for next bit */
        BB_GPIO_PIN_RESET(dev->scl);
        BB_DELAY_TICKS(dev->t_hold_ticks);
    }

    /* HIGH == NACK, LOW == ACK*/
    ack ? BB_GPIO_PIN_RESET(dev->sda) : BB_GPIO_PIN_SET(dev->sda);
    BB_DELAY_TICKS(dev->t_hold_ticks);

    /* release SCL and process clock stretch */
    if (!detect_high(dev->scl, dev->timeout_ticks))
        return false;

    /*SCL LOW for next byte */
    BB_GPIO_PIN_RESET(dev->scl);
    BB_DELAY_TICKS(dev->t_hold_ticks);

    return true;
}

bool i2c_bb_write(i2c_bb_device_t *dev, const uint8_t *data, uint32_t len, bool stop)
{
    setup_adaptive_timing(dev);
    
    i2c_bb_start(dev);

    if (!i2c_bb_write_byte(dev, (dev->addr << 1) | 0x00))
        return false;

    for (uint32_t i = 0; i < len; i++)
    {
        if (!i2c_bb_write_byte(dev, data[i]))
            return false;
    }

    if (stop)
        i2c_bb_stop(dev);

    return true;
}

bool i2c_bb_read(i2c_bb_device_t *dev, uint8_t *buf, uint32_t len, bool stop)
{
    i2c_bb_start(dev);

    if (!i2c_bb_write_byte(dev, (dev->addr << 1) | 0x01))
        return false;

    for (uint32_t i = 0; i < len; i++)
    {
        if (!i2c_bb_read_byte(dev, buf + i, i < (len - 1)))
            return false;
    }

    if (stop)
        i2c_bb_stop(dev);

    return true;
}

void i2c_bb_init(i2c_bb_device_t *dev, gpio_pin_t sda, gpio_pin_t scl, uint8_t addr)
{
    *dev = (i2c_bb_device_t){0};
    dev->sda = sda;
    dev->scl = scl;
    dev->addr = addr;

    BB_TICKS_SOURCE_INIT();
    BB_GPIO_INIT_OPEN_DRAIN(sda);
    BB_GPIO_INIT_OPEN_DRAIN(scl);

    setup_adaptive_timing(dev);
}
