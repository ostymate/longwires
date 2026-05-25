/**
 * @file bb_i2c.c
 * @brief Bit-banged I2C implementation with adaptive timing for long wires and slow devices,
 * including clock stretching support and performance monitoring features.
 * This implementation provides robust I2C communication by dynamically adjusting timing parameters based on actual signal rise times,
 * ensuring reliable operation even in challenging conditions.
 * It also includes features for monitoring the effective data transfer rates and timing characteristics,
 * which can be useful for debugging and optimizing performance.
 * The code is designed to be portable and can be adapted to various platforms by implementing the required GPIO and timing macros/functions.
 */

#include "bb_i2c.h"

#define DEFAULT_I2C_FREQ_HZ 100000UL
#define MAX_I2C_FREQ_HZ 1000000UL /* 1MHz freq limit */
#define INIT_I2C_FREQ_HZ 1000UL   /* initial frequency for timing setup - needs to be low enough to provide stable timing even with long wires and slow devices during timing setup */
#define MIN_LEVEL_SAMPLES 10      /* minimum samples to read for level detection to provide stable reading even with noise on the line */

#define T_RISE_TIMEOUT_US 25000 /* 25ms timeout for level stabilization and processing clock stretching */

bool set_level(i2c_bb_device_t *device, gpio_pin_t pin, uint32_t level, uint32_t *t_stab_measured_ticks)
{
    uint32_t start_tick, current_tick, expected_level_start_tick = 0;
    uint32_t t_rise_timeout_ticks = BB_US_TO_TICKS(T_RISE_TIMEOUT_US);
    uint32_t successful_samples = 0;

    level ? BB_GPIO_PIN_SET(pin) : BB_GPIO_PIN_RESET(pin);
    BB_GET_TICKS(start_tick);
    while (1)
    {
        BB_GET_TICKS(current_tick);

        if ((current_tick - start_tick) >= t_rise_timeout_ticks)
        {
            return false; /* timeout waiting for level to stabilize, return false to indicate failure */
        }

        successful_samples = (BB_GPIO_PIN_READ(pin) == level) ? (successful_samples + 1) : 0; /* reset successful samples count if we read different level */

        if (successful_samples)
        {
            if (successful_samples == 1)
                expected_level_start_tick = current_tick; /* update expected level start tick if we read the fist succesful sample of the desired level */

            if (successful_samples >= MIN_LEVEL_SAMPLES && (current_tick - expected_level_start_tick) >= device->t_hold_ticks) /* if we have enough successful samples, we can consider that level is detected and stabilized */
            {
                break;
            }
        }
    }

    if (successful_samples && t_stab_measured_ticks)
        *t_stab_measured_ticks = expected_level_start_tick - start_tick; /* calculate stabilization time */

    if (successful_samples && ((current_tick - expected_level_start_tick) != device->t_hold_ticks))
    {
        uint32_t user_defined_hold_ticks = (CPU_CLOCK_FREQ_HZ / device->freq_hz) / 2;                                   /* calculate user defined half clock period in ticks for timing setup */
        uint32_t new_t_hold_ticks = (current_tick - expected_level_start_tick + device->t_hold_ticks) / 2;              /* if level sampling time is more than current hold ticks, update hold ticks to provide stable timing */
        device->t_hold_ticks = new_t_hold_ticks > user_defined_hold_ticks ? new_t_hold_ticks : user_defined_hold_ticks; /* use higher value between measured stabilization time and user defined hold ticks for stable timing */
        uint32_t t_worst_rise = (device->t_rise_sda_ticks > device->t_rise_scl_ticks)
                                    ? device->t_rise_sda_ticks
                                    : device->t_rise_scl_ticks;
        device->actual_freq_hz = CPU_CLOCK_FREQ_HZ / ((device->t_hold_ticks * 2) + t_worst_rise);
    }

    return successful_samples ? true : false; /* return true if we successfully set the level and it stabilized, otherwise return false */
}

bool read_level(i2c_bb_device_t *device, gpio_pin_t pin)
{
    uint32_t start_tick, current_tick, high_level_votes, low_level_votes;
    high_level_votes = low_level_votes = 0;
    BB_GET_TICKS(start_tick);
    while (1)
    {
        BB_GET_TICKS(current_tick);
        if ((current_tick - start_tick) >= device->t_hold_ticks)
            break;

        if (BB_GPIO_PIN_READ(pin))
            high_level_votes++;
        else
            low_level_votes++;
    }
    return (high_level_votes > low_level_votes) ? true : false;
}

/* generate I2C start condition */
bool i2c_bb_start(i2c_bb_device_t *device)
{
    if (!set_level(device, device->sda_pin, 1, NULL))
        return false;
    if (!set_level(device, device->scl_pin, 1, NULL))
        return false;
    set_level(device, device->sda_pin, 0, NULL);
    set_level(device, device->scl_pin, 0, NULL);
    return true;
}

/* generate I2C stop condition */
bool i2c_bb_stop(i2c_bb_device_t *device)
{
    set_level(device, device->sda_pin, 0,  NULL);
    if (!set_level(device, device->scl_pin, 1,  NULL))
        return false;
    if (!set_level(device, device->sda_pin, 1,  NULL))
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
        if (((byte >> bit) & 1))
        {
            if (!set_level(device, device->sda_pin, 1, NULL)) // if bit is 1 set SDA HIGH, if failed - return false
                return false;
        }
        else
            set_level(device, device->sda_pin, 0, NULL); // if bit is 0 set SDA LOW

        if (!set_level(device, device->scl_pin, 1, NULL)) /* set SCL HIGH and process CS after bit set  */
            return false;

        set_level(device, device->scl_pin, 0, NULL); /* set SCL LOW for next bit */
    }

    set_level(device, device->sda_pin, 1, NULL); /* release SDA for ACK/NACK bit from slave */

    if (!set_level(device, device->scl_pin, 1, NULL)) /* process CS before ACK/NACK */
        return false;                                 /* clock stretching timeout */

    if (read_level(device, device->sda_pin)) /* receive ACK/NACK */
        return false;                        /* NACK received */

    set_level(device, device->scl_pin, 0, NULL); /* set SCL LOW for next byte */

    return true; /*  byte written and ACK received */
}

/* Read byte with ACK/NACK */
bool i2c_bb_read_byte(i2c_bb_device_t *device,
                      uint8_t *buffer, uint32_t send_ack)
{
    uint32_t bit = 8U;
    *buffer = 0U;

    set_level(device, device->sda_pin, 1, NULL);
    while (bit--) /* until all bits read or first clock stretching timeout */
    {
        if (!set_level(device, device->scl_pin, 1, NULL))                /* process CS before reading current bit */
            return false;                                                /* clock stretching timeout */
        *buffer |= ((read_level(device, device->sda_pin) & 1UL) << bit); /* read current bit */
        set_level(device, device->scl_pin, 0, NULL);                     /* set SCL LOW for next bit */
    }
    if (send_ack)
        set_level(device, device->sda_pin, 0, NULL); /* send ACK for all bytes except last one */
    else
        set_level(device, device->sda_pin, 1, NULL); /* send NACK for last byte */

    if (!set_level(device, device->scl_pin, 1, NULL)) /* wait for SCL HIGH or clock stretching timeout */
        return false;                                 /* clock stretching timeout */
    set_level(device, device->scl_pin, 0, NULL);      /* set SCL LOW for next byte */

    return true; /* byte read successfully */
}

void clean_bus(i2c_bb_device_t *device)
{
    uint32_t max_scl_toggles = 10;
    while (!(read_level(device, device->sda_pin) && read_level(device, device->scl_pin)))
    {
        set_level(device, device->scl_pin, 0, NULL);
        set_level(device, device->scl_pin, 1, NULL);
        if (max_scl_toggles-- == 0)
            break;
    }
    i2c_bb_stop(device);
}

void setup_adaptive_timing(i2c_bb_device_t *device)
{
    /*verify is user defined frequncy is more than 0 and less than MAX_I2C_FREQ_HZ */
    device->freq_hz = (device->freq_hz ? device->freq_hz : DEFAULT_I2C_FREQ_HZ);
    device->freq_hz = (device->freq_hz > MAX_I2C_FREQ_HZ) ? MAX_I2C_FREQ_HZ : device->freq_hz;

    /* use higher value between initial and user defined half clock ticks to provide stable timing even with long wires and slow devices during timing setup */
    uint32_t initial_hold_ticks = (CPU_CLOCK_FREQ_HZ / INIT_I2C_FREQ_HZ) / 2;                                           /* calculate initial half clock period in ticks for timing setup */
    uint32_t user_defined_hold_ticks = (CPU_CLOCK_FREQ_HZ / device->freq_hz) / 2;                                       /* calculate user defined half clock period in ticks for timing setup */
    device->t_hold_ticks = initial_hold_ticks > user_defined_hold_ticks ? initial_hold_ticks : user_defined_hold_ticks; /* use higher value (lower frequency) for stable timing */

    clean_bus(device);
    set_level(device, device->scl_pin, 0, NULL);
    set_level(device, device->sda_pin, 0, NULL);
    /* measure rise times to define timing parameters */
    set_level(device, device->sda_pin, 1, &device->t_rise_sda_ticks);
    set_level(device, device->scl_pin, 1, &device->t_rise_scl_ticks);

    uint32_t t_worst_rise_ticks = (device->t_rise_sda_ticks > device->t_rise_scl_ticks) ? device->t_rise_sda_ticks : device->t_rise_scl_ticks;

    device->t_hold_ticks = t_worst_rise_ticks > user_defined_hold_ticks ? t_worst_rise_ticks : user_defined_hold_ticks; /* use higher value between worst stabilization time and user defined hold ticks for stable timing */

    device->actual_freq_hz = CPU_CLOCK_FREQ_HZ / ((device->t_hold_ticks * 2) + t_worst_rise_ticks); /* calculate actual frequency based on adaptive timing setup */
}

bool i2c_bb_write(i2c_bb_device_t *device, const uint8_t *data, uint32_t len, bool write_stop)
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
    if (write_stop)
    {
        if (!i2c_bb_stop(device))
        {
            clean_bus(device);
            return false;
        }
    }

    return true;
}

bool i2c_bb_read(i2c_bb_device_t *device, uint8_t *buffer, uint32_t len, bool read_stop)
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
    if (read_stop)
    {
        if (!i2c_bb_stop(device))
        {
            clean_bus(device);
            return false;
        }
    }

    return true;
}

bool i2c_bb_transaction(i2c_bb_device_t *device,
                        const uint8_t *data, uint32_t data_len, uint8_t *buffer,
                        uint32_t buffer_len, bool write_stop, bool read_stop)
{
    if (!i2c_bb_write(device, data, data_len, write_stop))
        return false;
    if (!i2c_bb_read(device, buffer, buffer_len, read_stop))
        return false;
    return true;
}

void i2c_bb_init(i2c_bb_device_t *device)
{
    BB_TICKS_SOURCE_INIT();
    BB_GPIO_INIT_OPEN_DRAIN(device->sda_pin);
    BB_GPIO_INIT_OPEN_DRAIN(device->scl_pin);
    BB_GPIO_PIN_SET(device->sda_pin);
    BB_GPIO_PIN_SET(device->scl_pin);
    setup_adaptive_timing(device);
}

bool i2c_bb_device_available(i2c_bb_device_t *device)
{
    return i2c_bb_write(device, 0, 0, true);
}
