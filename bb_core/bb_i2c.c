/**
 * @file bb_i2c.c
 * @brief Bit-banged I2C. Works on 100m wires. Tolerates noise.
 *
 */

#include "bb_i2c.h"

#define I2C_BB_RISE_TIMEOUT_US  25000
#define I2C_BB_MIN_HOLD_US      5
#define MIN_SAMPLE_COUNT        5
#define CLEAN_BUS_MAX_ATTEMPTS  10

#define HIGH 1
#define LOW 0

static bool set_level(gpio_pin_t pin, bool level, uint32_t t_hold_ticks)
{
    uint32_t start_tick, current_tick, detected_tick = 0, samples = 0;
    uint32_t timeout = BB_US_TO_TICKS(I2C_BB_RISE_TIMEOUT_US);

    level ? BB_GPIO_PIN_SET(pin) : BB_GPIO_PIN_RESET(pin);
    BB_GET_TICKS(start_tick);

    while (1)
    {
        BB_GET_TICKS(current_tick);

        samples = (BB_GPIO_PIN_READ(pin) == level) ? samples + 1 : 0;
        if (samples == 1)
            detected_tick = current_tick;

        if (samples >= MIN_SAMPLE_COUNT && (current_tick - detected_tick) >= t_hold_ticks)
            return true;

        if ((current_tick - start_tick) >= timeout)
            return false;
    }
}

static bool wait_release(gpio_pin_t pin, uint32_t timeout_ticks)
{
    uint32_t start_tick, current_tick;

    BB_GET_TICKS(start_tick);
    while (1)
    {
        BB_GET_TICKS(current_tick);

        if (BB_GPIO_PIN_READ(pin))
            return true;

        if ((current_tick - start_tick) >= timeout_ticks)
            return false;
    }
}

void i2c_bb_start(i2c_bb_device_t *dev)
{
    set_level(dev->sda_pin, HIGH, dev->t_hold_ticks);
    set_level(dev->scl_pin, HIGH, dev->t_hold_ticks);
    set_level(dev->sda_pin, LOW,  dev->t_hold_ticks);
    set_level(dev->scl_pin, LOW,  dev->t_hold_ticks);
}

void i2c_bb_stop(i2c_bb_device_t *dev)
{
    set_level(dev->sda_pin, LOW,  dev->t_hold_ticks);
    set_level(dev->scl_pin, HIGH, dev->t_hold_ticks);
    set_level(dev->sda_pin, HIGH, dev->t_hold_ticks);
}

bool i2c_bb_write_byte(i2c_bb_device_t *dev, uint8_t byte)
{
    for (uint32_t bit = 8; bit--;)
    {
        set_level(dev->sda_pin, (byte >> bit) & 1, dev->t_hold_ticks);

        BB_GPIO_PIN_SET(dev->scl_pin);
        if (!wait_release(dev->scl_pin, dev->timeout_ticks))
            return false;

        set_level(dev->scl_pin, LOW, dev->t_hold_ticks);
    }

    /* ACK */
    BB_GPIO_PIN_SET(dev->sda_pin);

    BB_GPIO_PIN_SET(dev->scl_pin);
    if (!wait_release(dev->scl_pin, dev->timeout_ticks))
        return false;

    bool ack = !wait_release(dev->sda_pin, dev->t_hold_ticks);

    set_level(dev->scl_pin, LOW, dev->t_hold_ticks);
    return ack;
}

bool i2c_bb_read_byte(i2c_bb_device_t *dev, uint8_t *buf, bool ack)
{
    *buf = 0;
    BB_GPIO_PIN_SET(dev->sda_pin);

    for (uint32_t bit = 8; bit--;)
    {
        BB_GPIO_PIN_SET(dev->scl_pin);
        if (!wait_release(dev->scl_pin, dev->timeout_ticks))
            return false;

        *buf |= (wait_release(dev->sda_pin, dev->timeout_ticks) << bit);

        set_level(dev->scl_pin, LOW, dev->t_hold_ticks);
    }

    set_level(dev->sda_pin, !ack, dev->t_hold_ticks);

    BB_GPIO_PIN_SET(dev->scl_pin);
    if (!wait_release(dev->scl_pin, dev->timeout_ticks))
        return false;

    set_level(dev->scl_pin, LOW, dev->t_hold_ticks);
    return true;
}

void clean_bus(i2c_bb_device_t *dev)
{
	BB_GPIO_PIN_SET(dev->sda_pin);
	
    for (uint32_t i = 0; i < CLEAN_BUS_MAX_ATTEMPTS; i++)
    {
        set_level(dev->scl_pin, LOW,  dev->t_hold_ticks);
        set_level(dev->scl_pin, HIGH,  dev->t_hold_ticks);
    }

    i2c_bb_stop(dev);
}

void setup_adaptive_timing(i2c_bb_device_t *dev)
{
    uint32_t start, end, sda_rise, scl_rise;

    dev->timeout_ticks = BB_US_TO_TICKS(I2C_BB_RISE_TIMEOUT_US);
    dev->t_hold_ticks  = BB_US_TO_TICKS(I2C_BB_MIN_HOLD_US);

    clean_bus(dev);
	
    /* Both LOW first */
	  set_level(dev->scl_pin, LOW, dev->t_hold_ticks);
    set_level(dev->sda_pin, LOW, dev->t_hold_ticks);

    /* Release SDA, measure rise */
    BB_GET_TICKS(start);
    set_level(dev->sda_pin, HIGH, dev->t_hold_ticks);
    BB_GET_TICKS(end);
    sda_rise = end - start;

    /* Release SCL, measure rise */
    BB_GET_TICKS(start);
    set_level(dev->scl_pin, HIGH, dev->t_hold_ticks);
    BB_GET_TICKS(end);
    scl_rise = end - start;

    dev->t_hold_ticks = (sda_rise > scl_rise ? sda_rise : scl_rise);
	
	clean_bus(dev);
}

bool i2c_bb_write(i2c_bb_device_t *dev, const uint8_t *data, uint32_t len, bool stop)
{
    setup_adaptive_timing(dev);

	  i2c_bb_start(dev);
	
    if (!i2c_bb_write_byte(dev, (dev->addr << 1) | 0x00))
    {
        clean_bus(dev);
        return false;
    }

    for (uint32_t i = 0; i < len; i++)
    {
        if (!i2c_bb_write_byte(dev, data[i]))
        {
            clean_bus(dev);
            return false;
        }
    }

    if (stop) 
		  i2c_bb_stop(dev);

    return true;
}

bool i2c_bb_read(i2c_bb_device_t *dev, uint8_t *buf, uint32_t len, bool stop)
{
    i2c_bb_start(dev);

    if (!i2c_bb_write_byte(dev, (dev->addr << 1) | 0x01))
    {
        clean_bus(dev);
        return false;
    }

    for (uint32_t i = 0; i < len; i++)
    {
        if (!i2c_bb_read_byte(dev, buf + i, i < (len - 1)))
        {
            clean_bus(dev);
            return false;
        }
    }

    if (stop)
        i2c_bb_stop(dev);

    return true;
}

void i2c_bb_init(i2c_bb_device_t *dev, gpio_pin_t sda, gpio_pin_t scl, uint8_t addr)
{
    *dev = (i2c_bb_device_t){0};
    dev->sda_pin = sda;
    dev->scl_pin = scl;
    dev->addr    = addr;

    BB_TICKS_SOURCE_INIT();
    BB_GPIO_INIT_OPEN_DRAIN(sda);
    BB_GPIO_INIT_OPEN_DRAIN(scl);
	
    setup_adaptive_timing(dev);
}
