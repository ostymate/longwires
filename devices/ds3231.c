#include "ds3231.h"

#define DS3231_DEFAULT_ADDR 0x68

/* DS3231 registers */
#define REG_SECOND 0x00
#define REG_MINUTE 0x01
#define REG_HOUR 0x02
#define REG_WEEK 0x03
#define REG_DAY 0x04
#define REG_MONTH 0x05
#define REG_YEAR 0x06
#define REG_CONTROL 0x0E
#define REG_STATUS 0x0F

/* masks for filtering non data bits */
#define SECOND_MASK 0x7F
#define MINUTE_MASK 0x7F
#define HOUR_MASK 0x3F
#define DAY_MASK 0x3F
#define MONTH_MASK 0x1F
#define OSCILLATOR_FAILURE_MASK 0x80

#define YEAR_BASE 2000

#define BCD_TO_DEC(BCD) ((((BCD) >> 4) * 10) + ((BCD) & 0x0F))
#define DEC_TO_BCD(DEC) (((DEC) / 10) << 4) | ((DEC) % 10)

bool ds3231_init(ds3231_t *ds3231, gpio_pin_t sda_pin, gpio_pin_t scl_pin)
{
    if (!ds3231)
        return false;
    return i2c_init(&(ds3231->i2c_device), sda_pin, scl_pin, DS3231_DEFAULT_ADDR);
}

bool ds3231_get_time(ds3231_t *ds3231)
{

    if (!ds3231)
        return false;

    uint8_t buf[7];

    /* select REG_CONTROL (0x0E) */
    buf[0] = REG_CONTROL;
    if (!i2c_write(&ds3231->i2c_device, buf, 1, false))
        return false;

    /* read REG_CONTROL (0x0E) into buf[0] and REG_STATUS (0x0F) into buf[1] */
    if (!i2c_read(&ds3231->i2c_device, buf, 2, true))
        return false;

    /* check MSB both of them to find failures (!EOSC==1) in REG_CONTROL or (OSF==1) in REG_STATUS */
    if ((buf[0] | buf[1]) & OSCILLATOR_FAILURE_MASK)
        return false;

    /* select REG_SECOND (0x00) */
    buf[0] = REG_SECOND;
    if (!i2c_write(&ds3231->i2c_device, buf, 1, false))
        return false;

    /* reading REG_SECOND:REG_YEAR (0x00:0x06) */
    if (!i2c_read(&ds3231->i2c_device, buf, 7, true))
        return false;

    buf[REG_SECOND] = BCD_TO_DEC(buf[REG_SECOND] & SECOND_MASK);
    buf[REG_MINUTE] = BCD_TO_DEC(buf[REG_MINUTE] & MINUTE_MASK);
    buf[REG_HOUR] = BCD_TO_DEC(buf[REG_HOUR] & HOUR_MASK);
    buf[REG_DAY] = BCD_TO_DEC(buf[REG_DAY] & DAY_MASK);
    buf[REG_MONTH] = BCD_TO_DEC(buf[REG_MONTH] & MONTH_MASK);

    if (buf[REG_SECOND] > 59 || buf[REG_MINUTE] > 59 || buf[REG_HOUR] > 23)
        return false;
    if (buf[REG_DAY] < 1 || buf[REG_DAY] > 31)
        return false;
    if (buf[REG_WEEK] < 1 || buf[REG_WEEK] > 7)
        return false;
    if (buf[REG_MONTH] < 1 || buf[REG_MONTH] > 12)
        return false;
    if (buf[REG_YEAR] > 100)
        return false;

    ds3231->second = buf[REG_SECOND];
    ds3231->minute = buf[REG_MINUTE];
    ds3231->hour = buf[REG_HOUR];
    ds3231->day = buf[REG_DAY];
    ds3231->month = buf[REG_MONTH];
    ds3231->year = buf[REG_YEAR] + YEAR_BASE;

    return true;
}

bool ds3231_set_time(ds3231_t *ds3231)
{

    if (!ds3231)
        return false;
    if (ds3231->second > 59 || ds3231->minute > 59 || ds3231->hour > 23)
        return false;
    if (ds3231->day < 1 || ds3231->day > 31)
        return false;
    if (ds3231->month < 1 || ds3231->month > 12)
        return false;
    if ((ds3231->year - YEAR_BASE) > 100)
        return false;

    uint8_t cmd[] = {REG_SECOND, /* select REG SECOND 0x00 */
                     DEC_TO_BCD(ds3231->second),
                     DEC_TO_BCD(ds3231->minute),
                     DEC_TO_BCD(ds3231->hour),
                     1, /* not day of the week, just valid value (1-7) */
                     DEC_TO_BCD(ds3231->day),
                     DEC_TO_BCD(ds3231->month),
                     DEC_TO_BCD((uint8_t)(ds3231->year - YEAR_BASE))};

    if (!i2c_write(&ds3231->i2c_device, cmd, sizeof(cmd), true))
        return false;

    cmd[0] = REG_CONTROL; /* select REG_CONTROL */
    cmd[1] = 0;           /* enable oscillator (!EOSC=0) REG_CONTROL */
    cmd[2] = 0;           /* reset oscillator stop flag (OSF=0) REG_STATUS */

    if (!i2c_write(&ds3231->i2c_device, cmd, 3, true))
        return false;

    return true;
}