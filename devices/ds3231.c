#include "ds3231.h"

/* DS3231 registers */
#define REG_SECOND 0x00
#define REG_MINUTE 0x01
#define REG_HOUR 0x02
#define REG_DAY 0x04
#define REG_MONTH 0x05
#define REG_YEAR 0x06
#define REG_STATUS 0x0F
#define REG_CONTROL 0x0E

/* masks for filtering non data bits */
#define SECOND_MASK 0x7F
#define MINUTE_MASK 0x7F
#define HOUR_MASK 0x3F
#define DAY_MASK 0x3F
#define MONTH_MASK 0x1F
#define YEAR_MASK 0xFF
#define OSCILLATOR_STOP_FLAG_MASK 0x80

#define YEAR_BASE 2000

static inline uint8_t bcd_to_dec(uint8_t bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

static inline uint8_t dec_to_bcd(uint8_t dec)
{
    return ((dec / 10) << 4) | (dec % 10);
}

static bool read_reg(ds3231_t *ds3231, uint8_t reg, uint8_t *data)
{
    return i2c_bb_transaction(&(ds3231->ds3231_i2c_device), &reg, 1, data, 1, false, true);
}

static bool write_reg(ds3231_t *ds3231, uint8_t reg, uint8_t data)
{
    uint8_t buffer[2] = {reg, data};
    return i2c_bb_write(&ds3231->ds3231_i2c_device, buffer, 2, true);
}

void ds3231_init(ds3231_t *ds3231, gpio_pin_t sda_pin, gpio_pin_t scl_pin, uint32_t i2c_freq)
{
    if (!ds3231)
        return;

    ds3231->ds3231_i2c_device.sda_pin = sda_pin;
    ds3231->ds3231_i2c_device.scl_pin = scl_pin;
    ds3231->ds3231_i2c_device.addr = DS3231_DEFAULT_ADDR;
    ds3231->ds3231_i2c_device.freq_hz = i2c_freq ? i2c_freq : DS3231_DEFAULT_FREQ_HZ;
    i2c_bb_init(&(ds3231->ds3231_i2c_device));
    ds3231->is_time_valid = false;
}

void ds3231_get_time(ds3231_t *ds3231)
{

    if (!ds3231)
        return;

    ds3231->is_time_valid = false;

    uint8_t sec_reg, min_reg, hour_reg, day_reg, month_reg, year_reg, status_reg, control_reg;

    if (!read_reg(ds3231, REG_SECOND, &sec_reg))
        return;
    if (!read_reg(ds3231, REG_MINUTE, &min_reg))
        return;
    if (!read_reg(ds3231, REG_HOUR, &hour_reg))
        return;
    if (!read_reg(ds3231, REG_DAY, &day_reg))
        return;
    if (!read_reg(ds3231, REG_MONTH, &month_reg))
        return;
    if (!read_reg(ds3231, REG_YEAR, &year_reg))
        return;
    if (!read_reg(ds3231, REG_STATUS, &status_reg))
        return;
    if (!read_reg(ds3231, REG_CONTROL, &control_reg))
        return;

    if ((status_reg | control_reg) & OSCILLATOR_STOP_FLAG_MASK)
        return;

    uint8_t second = bcd_to_dec(sec_reg & SECOND_MASK);
    uint8_t minute = bcd_to_dec(min_reg & MINUTE_MASK);
    uint8_t hour = bcd_to_dec(hour_reg & HOUR_MASK);
    uint8_t day = bcd_to_dec(day_reg & DAY_MASK);
    uint8_t month = bcd_to_dec(month_reg & MONTH_MASK);
    uint16_t year = bcd_to_dec(year_reg & YEAR_MASK) + YEAR_BASE;

    if (!((second <= 59) && (minute <= 59) && (hour <= 23) && (day <= 31) && (month <= 12) && (year <= 2099)))
        return;

    ds3231->second = second;
    ds3231->minute = minute;
    ds3231->hour = hour;
    ds3231->day = day;
    ds3231->month = month;
    ds3231->year = year;
    ds3231->is_time_valid = true;
}

void ds3231_set_time(ds3231_t *ds3231)
{

    if (!ds3231)
        return;

    ds3231->is_time_valid = false;

    if (!write_reg(ds3231, REG_SECOND, dec_to_bcd(ds3231->second)))
        return;
    if (!write_reg(ds3231, REG_MINUTE, dec_to_bcd(ds3231->minute)))
        return;
    if (!write_reg(ds3231, REG_HOUR, dec_to_bcd(ds3231->hour)))
        return;
    if (!write_reg(ds3231, REG_DAY, dec_to_bcd(ds3231->day)))
        return;
    if (!write_reg(ds3231, REG_MONTH, dec_to_bcd(ds3231->month)))
        return;
    if (!write_reg(ds3231, REG_YEAR, dec_to_bcd((uint8_t)(ds3231->year - YEAR_BASE))))
        return;

    // reset osf
    if (!write_reg(ds3231, REG_STATUS, 0))
        return;
    if (!write_reg(ds3231, REG_CONTROL, 0))
        return;

    ds3231->is_time_valid = true;
}