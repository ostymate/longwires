
#include "bb_i2c.h"

#define DEFAULT_I2C_FREQ_HZ 100000UL
#define MAX_I2C_FREQ_HZ 1000000UL /* 1MHz freq limit */
#define INIT_I2C_FREQ_HZ 1000UL   /* initial frequency for timing setup - needs to be low enough to provide stable timing even with long wires and slow devices during timing setup */

#define T_RISE_TIMEOUT_US 25000 /* 25ms timeout for level stabilization and processing clock stretching */

bool set_level(gpio_pin_t pin, uint32_t t_hold_ticks, uint32_t level, uint32_t *t_rise_ticks)
{
    uint32_t start_tick, current_tick, expected_level_start_tick = 0;
    uint32_t t_rise_timeout_ticks = BB_US_TO_TICKS(T_RISE_TIMEOUT_US);
    bool level_detected = false;

    level ? BB_GPIO_PIN_SET(pin) : BB_GPIO_PIN_RESET(pin);
    BB_GET_TICKS(start_tick);
    while (1)
    {
        BB_GET_TICKS(current_tick);

        if ((current_tick - start_tick) >= t_rise_timeout_ticks)
        {
            level_detected = false;
            break;
        }

        if (BB_GPIO_PIN_READ(pin) == level)
        {
            if (!level_detected)
            {
                level_detected = true;
                expected_level_start_tick = current_tick;
                continue;
            }
            if ((current_tick - expected_level_start_tick) >= t_hold_ticks)
                break;
        }
        else
            level_detected = false;
    }

    if (level_detected && t_rise_ticks && level)
        *t_rise_ticks = expected_level_start_tick - start_tick; /* calculate rise time for HIGH level*/

    return level_detected;
}

bool read_level(gpio_pin_t pin, uint32_t t_hold_ticks)
{
    uint32_t start_tick, current_tick, high_level_votes, low_level_votes;
    high_level_votes = low_level_votes = 0;
    BB_GET_TICKS(start_tick);
    while (1)
    {
        BB_GET_TICKS(current_tick);
        if ((current_tick - start_tick) >= t_hold_ticks)
            break;

        if (BB_GPIO_PIN_READ(pin))
            high_level_votes++;
        else
            low_level_votes++;
    }
    return (high_level_votes >= low_level_votes) ? true : false;
}

/* generate I2C start condition */
void i2c_bb_start(i2c_bb_device_t *device)
{
    set_level(device->sda_pin, device->t_hold_ticks, 1, NULL);
    set_level(device->scl_pin, device->t_hold_ticks, 1, NULL);
    set_level(device->sda_pin, device->t_hold_ticks, 0, NULL);
    set_level(device->scl_pin, device->t_hold_ticks, 0, NULL);
}

/* generate I2C stop condition */
void i2c_bb_stop(i2c_bb_device_t *device)
{
    set_level(device->sda_pin, device->t_hold_ticks, 0, NULL);
    set_level(device->scl_pin, device->t_hold_ticks, 1, NULL);
    set_level(device->sda_pin, device->t_hold_ticks, 1, NULL);
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
            if (!set_level(device->sda_pin, device->t_hold_ticks, 1, NULL)) // if bit is 1 set SDA HIGH, if failed - return false
                return false;
        }
        else
            set_level(device->sda_pin, device->t_hold_ticks, 0, NULL); // if bit is 0 set SDA LOW

        if (!set_level(device->scl_pin, device->t_hold_ticks, 1, NULL)) /* set SCL HIGH and process CS after bit set  */
            return false;

        set_level(device->scl_pin, device->t_hold_ticks, 0, NULL); /* set SCL LOW for next bit */
    }

    set_level(device->sda_pin, device->t_hold_ticks, 1, NULL); /* release SDA for ACK/NACK bit from slave */

    if (!set_level(device->scl_pin, device->t_hold_ticks, 1, NULL)) /* process CS before ACK/NACK */
        return false;                                               /* clock stretching timeout */

    if (read_level(device->sda_pin, device->t_hold_ticks)) /* receive ACK/NACK */
        return false;                                      /* NACK received */

    set_level(device->scl_pin, device->t_hold_ticks, 0, NULL); /* set SCL LOW for next byte */

    return true; /*  byte written and ACK received */
}

/* Read byte with ACK/NACK */
bool i2c_bb_read_byte(i2c_bb_device_t *device,
                      uint8_t *buffer, uint32_t send_ack)
{
    uint32_t bit = 8U;
    *buffer = 0U;

    set_level(device->sda_pin, device->t_hold_ticks, 1, NULL);
    while (bit--) /* until all bits read or first clock stretching timeout */
    {
        if (!set_level(device->scl_pin, device->t_hold_ticks, 1, NULL))                /* process CS before reading current bit */
            return false;                                                              /* clock stretching timeout */
        *buffer |= ((read_level(device->sda_pin, device->t_hold_ticks) & 1UL) << bit); /* read current bit */
        set_level(device->scl_pin, device->t_hold_ticks, 0, NULL);                     /* set SCL LOW for next bit */
    }
    if (send_ack)
        set_level(device->sda_pin, device->t_hold_ticks, 0, NULL); /* send ACK for all bytes except last one */
    else
        set_level(device->sda_pin, device->t_hold_ticks, 1, NULL); /* send NACK for last byte */

    if (!set_level(device->scl_pin, device->t_hold_ticks, 1, NULL)) /* wait for SCL HIGH or clock stretching timeout */
        return false;                                               /* clock stretching timeout */
    set_level(device->scl_pin, device->t_hold_ticks, 0, NULL);      /* set SCL LOW for next byte */

    return true; /* byte read successfully */
}

void clean_bus(i2c_bb_device_t *device)
{
    uint32_t max_scl_toggles = 10;
    while (!(read_level(device->sda_pin, device->t_hold_ticks) && read_level(device->scl_pin, device->t_hold_ticks)))
    {
        set_level(device->scl_pin, device->t_hold_ticks, 0, NULL);
        set_level(device->scl_pin, device->t_hold_ticks, 1, NULL);
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

    uint32_t t_rise_sda_ticks, t_rise_scl_ticks;
    set_level(device->scl_pin, device->t_hold_ticks, 0, NULL);
    set_level(device->sda_pin, device->t_hold_ticks, 0, NULL);
    set_level(device->sda_pin, device->t_hold_ticks, 1, &t_rise_sda_ticks);
    set_level(device->scl_pin, device->t_hold_ticks, 1, &t_rise_scl_ticks);

    uint32_t t_worst_rise_ticks = (t_rise_sda_ticks > t_rise_scl_ticks) ? t_rise_sda_ticks : t_rise_scl_ticks;
    device->t_hold_ticks = t_worst_rise_ticks > user_defined_hold_ticks ? t_worst_rise_ticks : user_defined_hold_ticks; /* use higher value between worst rise time and user defined hold ticks for stable timing */
     
    device->actual_freq_hz = CPU_CLOCK_FREQ_HZ / ((device->t_hold_ticks * 2) + t_worst_rise_ticks);/* calculate actual frequency based on adaptive timing setup */
    device->t_rise_us = BB_TICKS_TO_US(t_worst_rise_ticks); /* calculate worst rise time in microseconds for debugging and performance monitoring purposes */
}

bool i2c_bb_write(i2c_bb_device_t *device, const uint8_t *data, uint32_t len, bool write_stop)
{
    setup_adaptive_timing(device);

    uint32_t start_tick, end_tick;
    BB_GET_TICKS(start_tick);

    i2c_bb_start(device);
    if (!i2c_bb_write_byte(device, (((device->addr) << 1U) | 0x00U)))
        return false;

    for (uint32_t i = 0UL; i < len; i++)
    {
        if (!i2c_bb_write_byte(device, data[i]))
            return false;
    }
    if (write_stop)
        i2c_bb_stop(device);

    BB_GET_TICKS(end_tick);
    uint32_t elapsed_ticks = end_tick - start_tick;

    device->bits_per_second_write = (device->bits_per_second_write + len * 8UL * CPU_CLOCK_FREQ_HZ / elapsed_ticks) / 2UL; /* calculate bits per second based on total bits sent and elapsed time. */

    return true;
}

bool i2c_bb_read(i2c_bb_device_t *device, uint8_t *buffer, uint32_t len, bool read_stop)
{
    uint32_t start_tick, end_tick;
    BB_GET_TICKS(start_tick);

    i2c_bb_start(device);
    if (!i2c_bb_write_byte(device, (((device->addr) << 1U) | 0x01U)))
        return false;
    for (uint32_t i = 0UL; i < len; i++)
    {
        // ACK for all except last
        if (!i2c_bb_read_byte(device, (buffer + i), (i < (len - 1UL))))
            return false;
    }
    if (read_stop)
        i2c_bb_stop(device);

    BB_GET_TICKS(end_tick);
    uint32_t elapsed_ticks = end_tick - start_tick;
    device->bits_per_second_read = (device->bits_per_second_read + len * 8UL * CPU_CLOCK_FREQ_HZ / elapsed_ticks) / 2UL; /* calculate bits per second based on total bits read and elapsed time. */

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
    device->bits_per_second_write = 0;
    device->bits_per_second_read = 0;
}

bool i2c_bb_device_available(i2c_bb_device_t *device)
{
    return i2c_bb_write(device, 0, 0, true);
}
