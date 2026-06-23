#include "ds18b20.h"

/* 1-Wire commands */
#define CMD_SKIP_ROM 0xCC
#define CMD_CONVERT_TEMP 0x44
#define CMD_READ_SCRATCHPAD 0xBE
#define CMD_WRITE_SCRATCHPAD 0x4E

/* temperature valid limits */
#define MIN_VALID_TEMP_X10 -550 // -55.0
#define MAX_VALID_TEMP_X10 1250 // 125.0

#define CONVERSION_NOT_STARTED UINT32_MAX
#define TEMP_NEVER_UPDATED UINT32_MAX

#define SCRATCHPAD_SIZE 9

/* scratchpad index */
#define TEMP_LSB 0
#define TEMP_MSB 1
#define TH_BYTE 2
#define TL_BYTE 3
#define CONFIG_BYTE 4
#define CRC_BYTE 8

#define TH_VALUE 0x7D     /* 125 `C */
#define TL_VALUE 0xC9     /* -55 `C  */
#define CONFIG_VALUE 0x7F /* 12 bit */

/* 750ms conversion interval (12-bit) */
#define CONVERSION_US 750000

static void start_conversion(ds18b20_t *sensor)
{
   /* write scratchpad: TH, TL, config.
   config guarantees 12-bit resolution regardless of EEPROM state.
   custom TH/TL values serve dual purpose:
   1. scratchpad integrity check - any corruption changes these bytes
   2. power-on reset detection - factory TH/TL differ from ours,
      so matching values prove a conversion has taken place.
      this makes 85.0 `C a valid measurable temperature. */
    if (!onewire_check_presence(sensor->data_pin))
    {
        sensor->conversion_started = CONVERSION_NOT_STARTED;
        return;
    }
    onewire_write_byte(sensor->data_pin, CMD_SKIP_ROM);
    onewire_write_byte(sensor->data_pin, CMD_WRITE_SCRATCHPAD);
    onewire_write_byte(sensor->data_pin, TH_VALUE);
    onewire_write_byte(sensor->data_pin, TL_VALUE);
    onewire_write_byte(sensor->data_pin, CONFIG_VALUE);

    /* start conversion */
    if (!onewire_check_presence(sensor->data_pin))
    {
        sensor->conversion_started = CONVERSION_NOT_STARTED;
        return;
    }
    onewire_write_byte(sensor->data_pin, CMD_SKIP_ROM);
    onewire_write_byte(sensor->data_pin, CMD_CONVERT_TEMP);
    
    /* set conversion started tick */
    GET_TICK(sensor->conversion_started);
    if (sensor->conversion_started == CONVERSION_NOT_STARTED)
        sensor->conversion_started++;
}

static void update_temp(ds18b20_t *sensor)
{

    if (!onewire_check_presence(sensor->data_pin))
        return;

    onewire_write_byte(sensor->data_pin, CMD_SKIP_ROM);
    onewire_write_byte(sensor->data_pin, CMD_READ_SCRATCHPAD);

    uint8_t scratchpad[SCRATCHPAD_SIZE];
    for (int i = 0; i < SCRATCHPAD_SIZE; i++)
        scratchpad[i] = onewire_read_byte(sensor->data_pin);

    /* check crc8 first*/
    if (!(onewire_crc8(scratchpad, 8) == scratchpad[CRC_BYTE]))
        return;

    /* verify TH, TL and config values */
    if (scratchpad[TH_BYTE] != TH_VALUE || scratchpad[TL_BYTE] != TL_VALUE || scratchpad[CONFIG_BYTE] != CONFIG_VALUE)
        return;

    int16_t temp_x10 = ((scratchpad[TEMP_MSB] << 8) | scratchpad[TEMP_LSB]) * 10 / 16;

    /* check validity range */
    if (temp_x10 < MIN_VALID_TEMP_X10 || temp_x10 > MAX_VALID_TEMP_X10)
        return;

    /* update temp and last valid timestamp*/
    sensor->temp_x10 = temp_x10;
    GET_TICK(sensor->last_temp_update);
    if (sensor->last_temp_update == TEMP_NEVER_UPDATED)
        sensor->last_temp_update++;

}

bool ds18b20_init(ds18b20_t *sensor, gpio_pin_t data_pin)
{

    sensor->data_pin = data_pin;
    sensor->temp_x10 = DS18B20_TEMP_NOT_MEASURED;
    sensor->last_temp_update = TEMP_NEVER_UPDATED;
    sensor->conversion_started = CONVERSION_NOT_STARTED;
    TICK_INIT();
    start_conversion(sensor);
    return sensor->conversion_started != CONVERSION_NOT_STARTED;
}

bool ds18b20_update(ds18b20_t *sensor, float *temp)
{
    if (!sensor)
        return false;
    
    if (temp && sensor->last_temp_update != TEMP_NEVER_UPDATED) 
        *temp = (float)sensor->temp_x10 * 0.1f;

    if (sensor->conversion_started == CONVERSION_NOT_STARTED)
        start_conversion(sensor);
     
    uint32_t now;
    GET_TICK(now);

    if (now - sensor->conversion_started > US_TO_TICKS(CONVERSION_US))
    {
        update_temp(sensor);
        start_conversion(sensor);
    }


    return sensor->last_temp_update != TEMP_NEVER_UPDATED && sensor->conversion_started != CONVERSION_NOT_STARTED;
}