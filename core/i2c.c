/**
 * @file i2c.c
 * @brief Bit-banged I2C. Works on 100m wires. Tolerates noise. Adaptive timings implemented.
 * @note "Frequency" depends on rise time of the signals, but not faster than 100kHz.
 *
 */

#include "i2c.h"

#define I2C_RISE_TIMEOUT_US 25000
#define I2C_MIN_HOLD_US 5
#define LEVEL_CONFIRMATION_SAMPLES_COUNT 5
#define CLEAN_BUS_SCL_CYCLES 9

static bool detect_high(gpio_pin_t pin, uint32_t timeout_ticks)
{
    uint32_t start_tick, current_tick, low_samples = 0;

    GET_TICK(start_tick);
    while (1)
    {
        GET_TICK(current_tick);

        if (PIN_READ(pin))
            return true;
        
        low_samples++;

        if (((current_tick - start_tick) >= timeout_ticks) && low_samples >= LEVEL_CONFIRMATION_SAMPLES_COUNT)
            return false;
    }
}

static bool i2c_start(i2c_device_t *dev)
{
    /* release SCL to perform repeated start if needed. slave will release SDA holded LOW after last ACK when master releases SCL*/
    if (!detect_high(dev->sda, dev->t_hold_ticks) || !detect_high(dev->scl, dev->t_hold_ticks))
    {
        PIN_Z(dev->sda);
        PIN_Z(dev->scl);
        if (!detect_high(dev->scl, dev->timeout_ticks)) 
            return false;
        if (!detect_high(dev->sda, dev->timeout_ticks))
            return false;  
    }  

    delay_ticks(dev->t_hold_ticks);
    PIN_LOW(dev->sda);
    delay_ticks(dev->t_hold_ticks);
    PIN_LOW(dev->scl);
    delay_ticks(dev->t_hold_ticks);

    return true;
}

static void i2c_stop(i2c_device_t *dev)
{
    PIN_LOW(dev->sda);
    delay_ticks(dev->t_hold_ticks);
    PIN_Z(dev->scl);
    delay_ticks(dev->t_hold_ticks);
    PIN_Z(dev->sda);
    delay_ticks(dev->t_hold_ticks);
}

static void clean_bus(i2c_device_t *dev)
{
    PIN_Z(dev->sda);

    for (uint32_t i = 0; i < CLEAN_BUS_SCL_CYCLES; i++)
    {
        /* set SCL low */
        PIN_LOW(dev->scl);
        delay_ticks(dev->t_hold_ticks);

        /* release SCL and process possible clock stretch waiting SCL HIGH untill timeout */
        PIN_Z(dev->scl);
        detect_high(dev->scl, dev->timeout_ticks);
        delay_ticks(dev->t_hold_ticks);
    }

    i2c_stop(dev);
}

static uint32_t measure_t_rise(gpio_pin_t pin, uint32_t timeout_ticks, uint32_t t_hold_ticks)
{
    uint32_t start_tick, current_tick, detected_tick = 0, samples = 0;

    GET_TICK(start_tick);
    PIN_Z(pin);
    while (1)
    {
        GET_TICK(current_tick);

        samples = PIN_READ(pin) ? samples + 1 : 0;
        detected_tick = samples > 1 ? detected_tick : current_tick;

        if (((samples >= LEVEL_CONFIRMATION_SAMPLES_COUNT) && ((current_tick - detected_tick) >= t_hold_ticks)) || (current_tick - start_tick) >= timeout_ticks)
            return current_tick - start_tick;
    }
}

static void setup_timing(i2c_device_t *dev)
{
    dev->timeout_ticks = US_TO_TICKS(I2C_RISE_TIMEOUT_US);
    uint32_t min_hold_ticks = US_TO_TICKS(I2C_MIN_HOLD_US);
    dev->t_hold_ticks = min_hold_ticks;

    clean_bus(dev);

    PIN_LOW(dev->scl);
    delay_ticks(dev->t_hold_ticks);
    PIN_LOW(dev->sda);
    delay_ticks(dev->t_hold_ticks);

    /* Release SDA, measure rise */
    uint32_t sda_hold_adaptive = measure_t_rise(dev->sda, dev->timeout_ticks, dev->t_hold_ticks);
    /* Release SCL, measure rise */
    uint32_t scl_hold_adaptive = measure_t_rise(dev->scl, dev->timeout_ticks, dev->t_hold_ticks);

    dev->t_hold_ticks = (sda_hold_adaptive > scl_hold_adaptive ? sda_hold_adaptive : scl_hold_adaptive);

    clean_bus(dev);
}

static bool i2c_write_byte(i2c_device_t *dev, uint8_t byte)
{
    for (uint32_t bit = 8; bit--;)
    {
        /* set current bit */
        if ((byte >> bit) & 1)
            PIN_Z(dev->sda);
        else 
            PIN_LOW(dev->sda);
        delay_ticks(dev->t_hold_ticks);

        /* release SCL and wait HIGH processing clock stretch*/
        PIN_Z(dev->scl);
        if (!detect_high(dev->scl, dev->timeout_ticks))
            return false;
        delay_ticks(dev->t_hold_ticks);

        /*SCL LOW for next bit*/
        PIN_LOW(dev->scl);
        delay_ticks(dev->t_hold_ticks);
    }

    /* release SDA for ACK / NACK */
    PIN_Z(dev->sda);
    delay_ticks(dev->t_hold_ticks);

    /* process clock stretch before ACK / NACK */
    PIN_Z(dev->scl);
    if (!detect_high(dev->scl, dev->timeout_ticks))
        return false;

    /* HIGH == NACK */
    if (detect_high(dev->sda, dev->t_hold_ticks))
        return false;

    /* SCL LOW for next byte */
    PIN_LOW(dev->scl);
    delay_ticks(dev->t_hold_ticks);

    return true;
}

static bool i2c_read_byte(i2c_device_t *dev, uint8_t *buf, bool ack)
{
    *buf = 0;

    /* release SDA before reading */ 
    PIN_Z(dev->sda);
    delay_ticks(dev->t_hold_ticks);
    
    for (uint32_t bit = 8; bit--;)
    {
        /* process clock stretch */
        PIN_Z(dev->scl);
        if (!detect_high(dev->scl, dev->timeout_ticks))
            return false;

        /* read current bit */
        *buf |= (detect_high(dev->sda, dev->t_hold_ticks) << bit);
        delay_ticks(dev->t_hold_ticks);
        
        /* set SCL LOW for next bit */
        PIN_LOW(dev->scl);
        delay_ticks(dev->t_hold_ticks);
    }

    /* HIGH == NACK, LOW == ACK*/
    if (ack) 
        PIN_LOW(dev->sda);
    else
        PIN_Z(dev->sda);
    delay_ticks(dev->t_hold_ticks);

    /* release SCL and process clock stretch */
    PIN_Z(dev->scl);
    if (!detect_high(dev->scl, dev->timeout_ticks))
        return false;
    delay_ticks(dev->t_hold_ticks); 

    /*SCL LOW for next byte */
    PIN_LOW(dev->scl);

    return true;
}

bool i2c_write(i2c_device_t *dev, const uint8_t *data, uint32_t len, bool stop)
{
    setup_timing(dev);
    
    if (!i2c_start(dev))
        return false;

    if (!i2c_write_byte(dev, (dev->addr << 1) | 0x00))
        return false;

    for (uint32_t i = 0; i < len; i++)
    {
        if (!i2c_write_byte(dev, data[i]))
            return false;
    }

    if (stop)
        i2c_stop(dev);

    return true;
}

bool i2c_read(i2c_device_t *dev, uint8_t *buf, uint32_t len, bool stop)
{
    if (!i2c_start(dev))
        return false;

    if (!i2c_write_byte(dev, (dev->addr << 1) | 0x01))
        return false;

    for (uint32_t i = 0; i < len; i++)
    {
        if (!i2c_read_byte(dev, buf + i, i < (len - 1)))
            return false;
    }

    if (stop)
        i2c_stop(dev);

    return true;
}

void i2c_init(i2c_device_t *dev, gpio_pin_t sda, gpio_pin_t scl, uint8_t addr)
{
    *dev = (i2c_device_t){0};
    dev->sda = sda;
    dev->scl = scl;
    dev->addr = addr;
    TICK_INIT();
    setup_timing(dev);
}
