/**
 * @file bb_i2c.c
 * @brief Bit-banged I2C implementation with adaptive timing for long wires and slow devices,
 * including clock stretching support and performance monitoring features.
 * This implementation provides robust I2C communication by dynamically adjusting timing parameters based on actual signal rise times,
 * ensuring reliable operation even in challenging conditions.
 * The code is designed to be portable and can be adapted to various platforms by implementing the required GPIO and timing macros/functions.
 */

#include "bb_i2c.h"

#define I2C_BB_RISE_TIMEOUT_US 25000 /* timeout for SCL and SDA rise (also includes clock stretch timeout) */
#define I2C_BB_MIN_LEVEL_HOLD_US 5 /* minimal hold level time for both lines */
#define MIN_SAMPLE_COUNT 5 /* level confirmation count (protection against interrupts and context switching)*/
#define CLEAN_BUS_MAX_ATTEMPTS 10 /* max count for scl toggles when cleaning bus */

bool set_level(gpio_pin_t pin, uint32_t level, uint32_t t_hold_ticks)
{
    uint32_t start_tick, current_tick,
        level_detected_tick = 0,
        successful_samples = 0,
        set_timeout_ticks = BB_US_TO_TICKS(I2C_BB_RISE_TIMEOUT_US);

    BB_GET_TICKS(start_tick);
    level ? BB_GPIO_PIN_SET(pin) : BB_GPIO_PIN_RESET(pin);
    while (1)
    {
        BB_GET_TICKS(current_tick);

        /* (re-)start sampling when desired level detected */
        successful_samples = BB_GPIO_PIN_READ(pin) == level ? successful_samples + 1 : 0;

        /* remember tick of first successful sample  */
        level_detected_tick = successful_samples > 1 ? level_detected_tick : current_tick;

        if ((current_tick - level_detected_tick) >= t_hold_ticks && successful_samples >= MIN_SAMPLE_COUNT)
            return true;

        if ((current_tick - start_tick) >= set_timeout_ticks)
            return false; /* timeout */
    }
}

bool read_sda(gpio_pin_t sda_pin, uint32_t t_hold_ticks)
{
    uint32_t start_tick, current_tick,
        high_level_votes = 0,
        low_level_votes = 0,
        level_detected_tick = 0,
        successful_samples = 0,
        read_timeout_ticks = BB_US_TO_TICKS(I2C_BB_RISE_TIMEOUT_US);

    bool last_level = BB_GPIO_PIN_READ(sda_pin);

    BB_GET_TICKS(start_tick);
    while (1)
    {
        BB_GET_TICKS(current_tick);

        bool current_level = BB_GPIO_PIN_READ(sda_pin);
        /* (re-)start sampling when read level changed */
        successful_samples = (current_level == last_level) ? successful_samples + 1 : 0;
        last_level = current_level;

        /* remember tick of first successful sample  */
        level_detected_tick = (successful_samples > 1) ? level_detected_tick : current_tick;

        if ((current_tick - level_detected_tick) >= t_hold_ticks && successful_samples >= MIN_SAMPLE_COUNT)
            return current_level;

        if (current_level)
            high_level_votes += successful_samples;
        else
            low_level_votes += successful_samples;

        /* if timeout reached fallback to voting */
        if ((current_tick - start_tick) >= read_timeout_ticks)
            break;
    }
    return high_level_votes >= low_level_votes;
}

/* generate I2C start condition */
bool i2c_bb_start(i2c_bb_device_t *device)
{
    /* release SDA and SCL if failed bus is busy */
    if (!set_level(device->sda_pin, 1, device->t_hold_ticks))
        return false;
    if (!set_level(device->scl_pin, 1, device->t_hold_ticks))
        return false;

    /* set SDA LOW */
    set_level(device->sda_pin, 0, device->t_hold_ticks);

    /* set SCL LOW*/
    set_level(device->scl_pin, 0, device->t_hold_ticks);

    return true;
}

/* generate I2C stop condition */
bool i2c_bb_stop(i2c_bb_device_t *device)
{
    /* set SDA LOW */
    set_level(device->sda_pin, 0, device->t_hold_ticks);

    /* wait SCL HIGH */
    if (!set_level(device->scl_pin, 1, device->t_hold_ticks))
        return false;

    /* release SDA and wait high */
    if (!set_level(device->sda_pin, 1, device->t_hold_ticks))
        return false;

    return true;
}

/* write byte with clock stretching and ACK/NACK detection */
bool i2c_bb_write_byte(i2c_bb_device_t *device, uint8_t byte)
{
    uint32_t bit = 8U;

    while (bit--) /* until all bits will be written or first CS timeout */
    {
        /* set current bit */
        if (!set_level(device->sda_pin, ((byte >> bit) & 1), device->t_hold_ticks))
            return false;

        if (!set_level(device->scl_pin, 1, device->t_hold_ticks)) /* set SCL HIGH and process CS after bit set  */
            return false;                                         /* clock stretch timeout */

        set_level(device->scl_pin, 0, device->t_hold_ticks); /* set SCL LOW for next bit */
    }

    BB_GPIO_PIN_SET(device->sda_pin); /* release SDA for ACK/NACK detection*/

    if (!set_level(device->scl_pin, 1, device->t_hold_ticks))
        return false; /* CS timeout  */

    if (read_sda(device->sda_pin, device->t_hold_ticks)) /* SDA HIGH == NACK, LOW == ACK*/
        return false;                      /* NACK received */

    set_level(device->scl_pin, 0, device->t_hold_ticks); /* set SCL LOW for next byte */

    return true; /*  byte written and ACK received */
}

/* Read byte with ACK/NACK */
bool i2c_bb_read_byte(i2c_bb_device_t *device,
                      uint8_t *buffer, uint32_t send_ack)
{
    uint32_t bit = 8U;
    *buffer = 0U;

    set_level(device->sda_pin, 1, device->t_hold_ticks);

    while (bit--) /* until all bits read or first clock stretching timeout */
    {
        if (!set_level(device->scl_pin, 1, device->t_hold_ticks))
            return false; /* clock stretch timeout */

        *buffer |= (read_sda(device->sda_pin, device->t_hold_ticks) << bit); /* read current bit */

        set_level(device->scl_pin, 0, device->t_hold_ticks); /* set SCL LOW for next bit */
    }

    if (send_ack)
        set_level(device->sda_pin, 0, device->t_hold_ticks); /* send ACK for all bytes except last one */
    else
        set_level(device->sda_pin, 1, device->t_hold_ticks); /* send NACK for last byte */

    if (!set_level(device->scl_pin, 1, device->t_hold_ticks)) /* wait for SCL HIGH or clock stretching timeout */
        return false;                                         /* clock stretching timeout */

    set_level(device->scl_pin, 0, device->t_hold_ticks); /* set SCL LOW for next byte */

    return true; /* byte read successfully */
}

void clean_bus(i2c_bb_device_t *device)
{

    uint32_t attempts = 0;
    if (read_sda(device->sda_pin, device->t_hold_ticks))
        return;
    while (!set_level(device->sda_pin, 1, device->t_hold_ticks))
    {

        set_level(device->scl_pin, 0, device->t_hold_ticks);
        set_level(device->scl_pin, 1, device->t_hold_ticks);
        if (attempts >= CLEAN_BUS_MAX_ATTEMPTS)
            break;
        attempts++;
    }
    i2c_bb_stop(device);
}

void setup_adaptive_timing(i2c_bb_device_t *device)
{

    device->t_hold_ticks = BB_US_TO_TICKS(I2C_BB_MIN_LEVEL_HOLD_US);

    clean_bus(device);
    set_level(device->scl_pin, 0, device->t_hold_ticks);
    set_level(device->sda_pin, 0, device->t_hold_ticks);

    /* measure rise times to define timing parameters */
    uint32_t rise_start_tick, rise_stop_tick;

    BB_GET_TICKS(rise_start_tick);
    set_level(device->sda_pin, 1, device->t_hold_ticks);
    BB_GET_TICKS(rise_stop_tick);
    uint32_t sda_rise_ticks = rise_stop_tick - rise_start_tick;

    BB_GET_TICKS(rise_start_tick);
    set_level(device->scl_pin, 1, device->t_hold_ticks);
    BB_GET_TICKS(rise_stop_tick);
    uint32_t scl_rise_ticks = rise_stop_tick - rise_start_tick;

    device->t_hold_ticks = sda_rise_ticks > scl_rise_ticks ? sda_rise_ticks : scl_rise_ticks;

    clean_bus(device);
}

bool i2c_bb_write(i2c_bb_device_t *device, const uint8_t *data, uint32_t len, bool stop)
{
    setup_adaptive_timing(device);

    if (!i2c_bb_start(device))
    {
        clean_bus(device);
        return false;
    }

    if (!i2c_bb_write_byte(device, (((device->addr) << 1U) | 0x00U)))
    {
        clean_bus(device);
        return false;
    }

    for (uint32_t i = 0UL; i < len; i++)
    {
        if (!i2c_bb_write_byte(device, data[i]))
        {
            clean_bus(device);
            return false;
        }
    }
    if (stop)
    {
        if (!i2c_bb_stop(device))
        {
            clean_bus(device);
            return false;
        }
    }

    return true;
}

bool i2c_bb_read(i2c_bb_device_t *device, uint8_t *buffer, uint32_t len, bool stop)
{

    if (!i2c_bb_start(device))
    {
        clean_bus(device);
        return false;
    }

    if (!i2c_bb_write_byte(device, (((device->addr) << 1U) | 0x01U)))
    {
        clean_bus(device);
        return false;
    }

    for (uint32_t i = 0UL; i < len; i++)
    {
        // ACK for all except last
        if (!i2c_bb_read_byte(device, (buffer + i), (i < (len - 1UL))))
        {
            clean_bus(device);
            return false;
        }
    }
    if (stop)
    {
        if (!i2c_bb_stop(device))
        {
            clean_bus(device);
            return false;
        }
    }

    return true;
}

void i2c_bb_init(i2c_bb_device_t *device, gpio_pin_t sda_pin, gpio_pin_t scl_pin, uint8_t addr)
{
    *device = (i2c_bb_device_t){0}; /* zero initialize device structure */
    device->sda_pin = sda_pin;
    device->scl_pin = scl_pin;
    device->addr = addr;
    BB_TICKS_SOURCE_INIT();
    BB_GPIO_INIT_OPEN_DRAIN(device->sda_pin);
    BB_GPIO_INIT_OPEN_DRAIN(device->scl_pin);
    BB_GPIO_PIN_SET(device->sda_pin);
    BB_GPIO_PIN_SET(device->scl_pin);
    setup_adaptive_timing(device);
}
