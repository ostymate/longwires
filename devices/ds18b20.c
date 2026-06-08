#include "ds18b20.h"

// 1-Wire commands
#define DS18B20_CMD_CONVERT_TEMP 0x44
#define DS18B20_CMD_READ_SCRATCHPAD 0xBE

// valid values
#define DS18B20_MIN_VALID_TEMP_X10 -550 // -55.0
#define DS18B20_MAX_VALID_TEMP_X10 1250 // 125.0
#define DS18B20_INIT_VALUE 850

#define DS18B20_CONVERSION_US 750000 // 750ms conversion time (12-bit)

void ds18b20_init(ds18b20_sensor_t *sensor, gpio_pin_t data_pin)
{

    sensor->data_pin = data_pin;
    sensor->is_active = false;
    sensor->is_converting = false;
    sensor->error_count = 0;
    sensor->success_count = 0;
    BB_GPIO_PIN_INIT(data_pin);
    BB_TICKS_SOURCE_INIT();
}

static void ds18b20_process_error(ds18b20_sensor_t *sensor)
{
    sensor->error_count++;
    sensor->is_active = false;
    sensor->is_converting = false;
}

void ds18b20_update(ds18b20_sensor_t *sensor)
{
    if (!sensor)
        return;

    uint32_t current_ticks;
    BB_GET_TICKS(current_ticks);

    if (sensor->is_converting)
    {
        if ((current_ticks - sensor->conversion_started) < BB_US_TO_TICKS(DS18B20_CONVERSION_US))
            return;

        
        //  Read temperature data
        if (onewire_check_presence(sensor->data_pin))
        {
            onewire_write_byte(sensor->data_pin, ONEWIRE_CMD_SKIP_ROM);
            onewire_write_byte(sensor->data_pin, DS18B20_CMD_READ_SCRATCHPAD);

            for (int i = 0; i < 9; i++)
            {
                sensor->scratchpad[i] = onewire_read_byte(sensor->data_pin);
            }

            // Validate and process data
            if (onewire_crc8(sensor->scratchpad, 8) == sensor->scratchpad[8])
            {
                int16_t temp = ((sensor->scratchpad[1] << 8) | sensor->scratchpad[0]) * 10 / 16;

                if (temp >= DS18B20_MIN_VALID_TEMP_X10 && temp <= DS18B20_MAX_VALID_TEMP_X10 && temp != DS18B20_INIT_VALUE)
                {
                    sensor->temperature_x10 = temp;
                    sensor->success_count++;
                    sensor->is_active = true;
                    sensor->is_converting = false;
                }
                else
                    ds18b20_process_error(sensor);
            }
            else
                ds18b20_process_error(sensor);
        }
        else
            ds18b20_process_error(sensor);
       
    }
    else
    {
        
        if (onewire_check_presence(sensor->data_pin))
        {
            onewire_write_byte(sensor->data_pin, ONEWIRE_CMD_SKIP_ROM);
            onewire_write_byte(sensor->data_pin, DS18B20_CMD_CONVERT_TEMP);
            BB_GET_TICKS(sensor->conversion_started);
            sensor->is_converting = true;
        }
        else
            ds18b20_process_error(sensor);
       
    }
}