
#include "bb_i2c.h"

/* Convert clock stretching timeout (us) to ticks */
#define CS_TIMEOUT_TICKS (BB_US_TO_TICKS((DEFAULT_CLOCK_STRETCHING_TIMEOUT_US)))

/* Avoid division by zero */
#define VALIDATE_I2C_FREQ(FREQ) ((FREQ) > 0UL ? (FREQ) : I2C_DEFAULT_FREQ_HZ)

/* Convert I2C frequecy to half bit delay ticks */
#define I2C_FREQ_HZ_TO_TICKS(I2C_FREQ_HZ) (CPU_CLOCK_FREQ_HZ / (VALIDATE_I2C_FREQ(I2C_FREQ_HZ) * 2UL))

/* I2C semantic macros for GPIO operations  */
#define SDA_HIGH(SDA_PIN) BB_GPIO_PIN_SET(SDA_PIN)
#define SDA_LOW(SDA_PIN) BB_GPIO_PIN_RESET(SDA_PIN)
#define SCL_HIGH(SCL_PIN) BB_GPIO_PIN_SET(SCL_PIN)
#define SCL_LOW(SCL_PIN) BB_GPIO_PIN_RESET(SCL_PIN)
#define SDA_READ(SDA_PIN) BB_GPIO_PIN_READ(SDA_PIN)
#define SCL_READ(SCL_PIN) BB_GPIO_PIN_READ(SCL_PIN)

/* read and write I2C commands */
#define I2C_WRITE_CMD(ADDR) (((ADDR) << 1U) | 0x00U)
#define I2C_READ_CMD(ADDR) (((ADDR) << 1U) | 0x01U)

/* generate I2C start condition */
static inline void i2c_bb_start(gpio_pin_t sda_pin, gpio_pin_t scl_pin, uint32_t half_bit_delay)
{
    SDA_HIGH(sda_pin);
    SCL_HIGH(scl_pin);
    BB_DELAY_TICKS(half_bit_delay);
    SDA_LOW(sda_pin);
    BB_DELAY_TICKS(half_bit_delay);
    SCL_LOW(scl_pin);
    BB_DELAY_TICKS(half_bit_delay);
}

/* generate I2C stop condition */
static inline void i2c_bb_stop(gpio_pin_t sda_pin, gpio_pin_t scl_pin, uint32_t half_bit_delay)
{
    SDA_LOW(sda_pin);
    BB_DELAY_TICKS(half_bit_delay);
    SCL_HIGH(scl_pin);
    BB_DELAY_TICKS(half_bit_delay);
    SDA_HIGH(sda_pin);
    BB_DELAY_TICKS(half_bit_delay);
}

/* detect is SCL HIGH or clock stretching timeout */
static inline uint32_t i2c_bb_process_clock_stretching(uint32_t timeout_ticks, gpio_pin_t scl_pin)
{
    uint32_t cs_start, cs_current, scl_state;
    BB_GET_TICKS(cs_start);
    do
    {
        scl_state = SCL_READ(scl_pin);
        BB_GET_TICKS(cs_current);
    } while ((!scl_state) && ((cs_current - cs_start) < timeout_ticks)); /* wait for SCL HIGH || CS timeout */
    return scl_state; /*  0x00000001 - SCL HIGH in time || 0x00000000 - CS timeout */
}

/* write byte with clock stretching and ACK/NACK detection */
static inline uint32_t i2c_bb_write_byte(gpio_pin_t sda_pin, gpio_pin_t scl_pin, uint32_t cs_timeout_ticks,
                                         uint32_t half_bit_delay_ticks, uint8_t byte)
{
    uint32_t success = 0x01U;
    uint32_t bit = 8U;

    while ((bit--) && success) /* until all bits will be written or first CS timeout */
    {

        ((byte >> bit) & 1) ? SDA_HIGH(sda_pin) : SDA_LOW(sda_pin); /* set current bit */
        SCL_HIGH(scl_pin);
        success &= i2c_bb_process_clock_stretching(cs_timeout_ticks, scl_pin); /* process CS after bit set */
        BB_DELAY_TICKS(half_bit_delay_ticks);
        SCL_LOW(scl_pin);
        BB_DELAY_TICKS(half_bit_delay_ticks);
    }

    SDA_HIGH(sda_pin);
    BB_DELAY_TICKS(half_bit_delay_ticks);
    SCL_HIGH(scl_pin);
    success &= i2c_bb_process_clock_stretching(cs_timeout_ticks, scl_pin); /* process CS before ACK/NACK */
    BB_DELAY_TICKS(half_bit_delay_ticks);
    success &= !SDA_READ(sda_pin); /* receive ACK/NACK */
    SCL_LOW(scl_pin);
    BB_DELAY_TICKS(half_bit_delay_ticks);

    return success; /* 0x00000001 - byte written and ACK received || 0x00000000 - NACK recieved or CS timeout */
}

/* Read byte with ACK/NACK */
static inline uint32_t i2c_bb_read_byte(gpio_pin_t sda_pin, gpio_pin_t scl_pin, uint32_t cs_timeout_ticks,
                                        uint32_t half_bit_delay, uint8_t *buffer, uint32_t send_ack)
{
    uint32_t success = 0x01U;
    uint32_t bit = 8U;
    *buffer = 0U;

    SDA_HIGH(sda_pin);
    while ((bit--) && success) /* until all bits read or first clock stretching timeout */
    {
        SCL_HIGH(scl_pin);
        success &= i2c_bb_process_clock_stretching(cs_timeout_ticks, scl_pin); /* process CS before reading current bit */
        BB_DELAY_TICKS(half_bit_delay);
        *buffer |= ((SDA_READ(sda_pin) & 1UL) << bit); /* read current bit */
        SCL_LOW(scl_pin);
        BB_DELAY_TICKS(half_bit_delay);
    }

    (send_ack ? SDA_LOW(sda_pin) : SDA_HIGH(sda_pin)); /* send ACK for all bytes except last one */
    BB_DELAY_TICKS(half_bit_delay);
    SCL_HIGH(scl_pin);
    success &= i2c_bb_process_clock_stretching(cs_timeout_ticks, scl_pin); /* wait for SCL HIGH or clock stretching timeout */
    BB_DELAY_TICKS(half_bit_delay);
    SCL_LOW(scl_pin);
    BB_DELAY_TICKS(half_bit_delay);

    return success; /*0x00000001 - byte read || 0x00000000 - cs timeout*/
}

bool i2c_bb_write(i2c_bb_device_t *i2c_bb_device, const uint8_t *data, uint32_t len, bool write_stop)
{
    gpio_pin_t sda_pin = i2c_bb_device->sda_pin;
    gpio_pin_t scl_pin = i2c_bb_device->scl_pin;
    uint8_t addr = i2c_bb_device->addr;
    uint32_t cs_timeout_ticks = CS_TIMEOUT_TICKS;

    BB_ENTER_CRITICAL();
    uint32_t half_bit_delay_ticks = I2C_FREQ_HZ_TO_TICKS(i2c_bb_device->freq_hz);

    i2c_bb_start(sda_pin, scl_pin, half_bit_delay_ticks);
    bool success = i2c_bb_write_byte(sda_pin, scl_pin, cs_timeout_ticks, half_bit_delay_ticks, I2C_WRITE_CMD(addr));

    for (uint32_t i = 0UL; i < len; i++)
    {
        success &= i2c_bb_write_byte(sda_pin, scl_pin, cs_timeout_ticks, half_bit_delay_ticks, data[i]);
        if (!success)
            break;
    }
    if (write_stop)
        i2c_bb_stop(sda_pin, scl_pin, half_bit_delay_ticks);
    BB_EXIT_CRITICAL();

    return success;
}

bool i2c_bb_read(i2c_bb_device_t *i2c_bb_device, uint8_t *buffer, uint32_t len, bool read_stop)
{
    gpio_pin_t sda_pin = i2c_bb_device->sda_pin;
    gpio_pin_t scl_pin = i2c_bb_device->scl_pin;
    uint8_t addr = i2c_bb_device->addr;
    uint32_t cs_timeout_ticks = CS_TIMEOUT_TICKS;

    BB_ENTER_CRITICAL();
    uint32_t half_bit_delay_ticks = I2C_FREQ_HZ_TO_TICKS(i2c_bb_device->freq_hz);
    i2c_bb_start(sda_pin, scl_pin, half_bit_delay_ticks);
    bool success = i2c_bb_write_byte(sda_pin, scl_pin, cs_timeout_ticks, half_bit_delay_ticks, I2C_READ_CMD(addr));
    for (uint32_t i = 0UL; i < len; i++)
    {
        // ACK for all except last
        success &= i2c_bb_read_byte(sda_pin, scl_pin, cs_timeout_ticks, half_bit_delay_ticks, (buffer + i), (i < (len - 1UL)));
        if (!success)
            break;
    }
    if (read_stop)
        i2c_bb_stop(sda_pin, scl_pin, half_bit_delay_ticks);
    BB_EXIT_CRITICAL();

    return success;
}

bool i2c_bb_transaction(i2c_bb_device_t *i2c_bb_device,
                        const uint8_t *data, uint32_t data_len, uint8_t *buffer,
                        uint32_t buffer_len, bool write_stop, bool read_stop)
{
    bool success = i2c_bb_write(i2c_bb_device, data, data_len, write_stop);
    success &= i2c_bb_read(i2c_bb_device, buffer, buffer_len, read_stop);
    return success;
}

void i2c_bb_init(i2c_bb_device_t *i2c_bb_device)
{
    BB_TICKS_SOURCE_INIT();
    BB_GPIO_INIT_OPEN_DRAIN(i2c_bb_device->sda_pin);
    BB_GPIO_INIT_OPEN_DRAIN(i2c_bb_device->scl_pin);
    SDA_HIGH(i2c_bb_device->sda_pin);
    SCL_HIGH(i2c_bb_device->scl_pin);
}

bool i2c_bb_device_available(i2c_bb_device_t *i2c_bb_device)
{
    return i2c_bb_write(i2c_bb_device, 0, 0, true);
}
