#include "ds18b20.h"

/* 1-Wire commands */
#define DS18B20_CMD_CONVERT_TEMP 0x44
#define DS18B20_CMD_READ_SCRATCHPAD 0xBE

/* temperature valid limits */
#define DS18B20_MIN_VALID_TEMP_X10 -550 // -55.0
#define DS18B20_MAX_VALID_TEMP_X10 1250 // 125.0

#define SCRATCHPAD_SIZE 9
#define CONVERSION_NEVER_STARTED UINT32_MAX

/* 750ms conversion interval (12-bit) */
#define DS18B20_CONVERSION_US 750000

static bool start_conversion(ds18b20_sensor_t *sensor)
{
    if (!onewire_check_presence(sensor->data_pin))
        return false;
    onewire_write_byte(sensor->data_pin, ONEWIRE_CMD_SKIP_ROM);
    onewire_write_byte(sensor->data_pin, DS18B20_CMD_CONVERT_TEMP);
    GET_TICK(sensor->conversion_started);
    return true;
}

static int16_t read_scratchpad(ds18b20_sensor_t *sensor)
{

    if (!onewire_check_presence(sensor->data_pin))
        return DS18B20_INIT_VALUE;

    onewire_write_byte(sensor->data_pin, ONEWIRE_CMD_SKIP_ROM);
    onewire_write_byte(sensor->data_pin, DS18B20_CMD_READ_SCRATCHPAD);

    uint8_t scratchpad[SCRATCHPAD_SIZE];
    for (int i = 0; i < SCRATCHPAD_SIZE; i++)
        scratchpad[i] = onewire_read_byte(sensor->data_pin);

    if (!(onewire_crc8(scratchpad, 8) == scratchpad[8]))
        return DS18B20_INIT_VALUE;

    int16_t temp_x10 = ((scratchpad[1] << 8) | scratchpad[0]) * 10 / 16;

    if (temp_x10 < DS18B20_MIN_VALID_TEMP_X10 || temp_x10 > DS18B20_MAX_VALID_TEMP_X10)
        return DS18B20_INIT_VALUE;

    return temp_x10;
}

bool ds18b20_init(ds18b20_sensor_t *sensor, gpio_pin_t data_pin)
{

    sensor->data_pin = data_pin;
    sensor->temp_x10 = DS18B20_INIT_VALUE;
    sensor->last_valid_conversion = DS18B20_TEMP_NEVER_UPDATED;
    sensor->conversion_started = CONVERSION_NEVER_STARTED;
    TICK_INIT();
    return start_conversion(sensor);
}

uint32_t ds18b20_update(ds18b20_sensor_t *sensor, float *temp)
{
    if (!sensor)
        return DS18B20_TEMP_NEVER_UPDATED;

    uint32_t now;
    GET_TICK(now);

    if (now - sensor->conversion_started > US_TO_TICKS(DS18B20_CONVERSION_US))
    {

        int16_t temp_x10 = read_scratchpad(sensor);
        start_conversion(sensor);

        if (temp_x10 != DS18B20_INIT_VALUE)
        {
            sensor->temp_x10 = temp_x10;
            GET_TICK(sensor->last_valid_conversion);
            GET_TICK(now);
        }
    }

    if (temp)
        *temp = (float)sensor->temp_x10 * 0.1f;

    if (sensor->last_valid_conversion == DS18B20_TEMP_NEVER_UPDATED && sensor->conversion_started == CONVERSION_NEVER_STARTED)
        return DS18B20_TEMP_NEVER_UPDATED;

    return (now - sensor->last_valid_conversion) / US_TO_TICKS(1);
}