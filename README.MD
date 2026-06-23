
# longwires

**Software bitbang I2C and 1-Wire library for long-distance wiring**


# Features

- Pure software bitbang: no hardware I2C/1-Wire peripheral needed
- Automatic frequency adaptation: up to 100 kHz, slows down as the line demands
- Long distance wiring: all devices tested with 100 meters twisted pair for each supported MCU
- Clock stretching support: handles slow slave devices with configurable timeout 
- Non-blocking DS18B20: state machine based, doesn't block your main loop
- Parasitic power DS18B20: protects device against self heating after ESD 
- DS18B20 full range measurement: 85 °C problem solved using scratchpad fingerprint
- ASCII text SSD1306 driver: lightweight buffer (129 bytes ram required)
- Fixed-point data: optional float support for SHT3X and DS18B20
- Platform abstraction: could be easily ported to any MCU 



# Supported MCU
    - ESP32 (Xtensa and RISC-V based) 
    - STM32F1 
    - AVR 
# Supported frameworks
    - ESP-IDF
    - Platformio
# Included drivers
    I2C devices:
        SHT3X
        DS3231
        SSD1306 
    1-Wire:
        DS18B20

# How to install
**ESP-IDF**  
*idf_component.yml:*
```yaml
dependencies:
  longwires:
    git: https://github.com/ostymate/longwires.git
    version: "main"
```

**PlatformIO**  
*platformio.ini*: 
```ini
lib_deps = 
    https://github.com/ostymate/longwires.git
```

# How to use
***ESP32 (ESP-IDF)***
```C
#include "ds18b20.h"
#include "sht3x.h"
#include "ssd1306.h"
#include "ds3231.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>

void app_main(void)
{
    /* ESP32 pin format: PIN_INIT(gpio_number) */
    gpio_pin_t sda = PIN_INIT(21);
    gpio_pin_t scl = PIN_INIT(22);
    gpio_pin_t ow_pin = PIN_INIT(23);

    ds18b20_t ds18b20;
    sht3x_t sht3x;
    ssd1306_t oled;
    ds3231_t rtc;

    char buf[128];
    uint32_t now;

    if (ssd1306_init(&oled, sda, scl, true, false))  /* true - default addr (0x3C),  false - don't flip display 180° */
        ssd1306_print(&oled, "Initializing...");
    vTaskDelay(pdMS_TO_TICKS(300));

    if (ds18b20_init(&ds18b20, ow_pin))
        ssd1306_print(&oled, "ds18b20: OK");
    else
        ssd1306_print(&oled, "ds18b20: FAIL");
    vTaskDelay(pdMS_TO_TICKS(300));

    if (sht3x_init(&sht3x, sda, scl, true)) /* true - default addr (0x44)  */
        ssd1306_print(&oled, "sht3x: OK");
    else
        ssd1306_print(&oled, "sht3x: FAIL");
    vTaskDelay(pdMS_TO_TICKS(300));

    if (ds3231_init(&rtc, sda, scl))
        ssd1306_print(&oled, "ds3231: OK");
    else
        ssd1306_print(&oled, "ds3231: FAIL");

    rtc.year = 2026;
    rtc.month = 6;
    rtc.day = 23;
    rtc.hour = 18;
    rtc.minute = 21;
    rtc.second = 0;
    ds3231_set_time(&rtc);

    while (1) {
        ds3231_get_time(&rtc);
        sht3x_read(&sht3x, NULL, NULL);
        ds18b20_update(&ds18b20, NULL);

        GET_TICK(now);
        
        /* calculate how data aged */
        uint32_t sht3x_data_age_ms = (now - sht3x.last_update) / US_TO_TICKS(1) / 1000;
        uint32_t ds18_temp_age_ms = (now - ds18b20.last_temp_update) / US_TO_TICKS(1) / 1000;

        snprintf(buf, sizeof(buf), "%u.%02u.%02u %02u:%02u:%02u\n\nSHT31 %d.%dC %u%%\n%lu ms ago\n\nDS18B20 %d.%dC\n%lu ms ago\n",
                 rtc.year, rtc.month, rtc.day, rtc.hour, rtc.minute, rtc.second,
                 sht3x.temp_x10 / 10, abs(sht3x.temp_x10 % 10), sht3x.hum, sht3x_data_age_ms,
                 ds18b20.temp_x10 / 10, abs(ds18b20.temp_x10 % 10), ds18_temp_age_ms);

        ssd1306_print(&oled, buf);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
```  

***STM32F1 CMSIS, no HAL***
```c
#include "ds18b20.h"
#include "sht3x.h"
#include "ssd1306.h"
#include "ds3231.h"
#include <stdio.h>

int main(void)
{
    /* STM32 pin format: PIN_INIT(GPIOx, pin_number) */
    gpio_pin_t sda = PIN_INIT(GPIOB, 6);
    gpio_pin_t scl = PIN_INIT(GPIOB, 7);
    gpio_pin_t ow_pin = PIN_INIT(GPIOB, 8);

    ds18b20_t ds18b20;
    sht3x_t sht3x;
    ssd1306_t oled;
    ds3231_t rtc;

    char buf[128];
    uint32_t now;

    if (ssd1306_init(&oled, sda, scl, true, false))
        ssd1306_print(&oled, "Initializing...");

    if (ds18b20_init(&ds18b20, ow_pin))
        ssd1306_print(&oled, "ds18b20: OK");
    else
        ssd1306_print(&oled, "ds18b20: FAIL");

    if (sht3x_init(&sht3x, sda, scl, true))
        ssd1306_print(&oled, "sht3x: OK");
    else
        ssd1306_print(&oled, "sht3x: FAIL");

    if (ds3231_init(&rtc, sda, scl))
        ssd1306_print(&oled, "ds3231: OK");
    else
        ssd1306_print(&oled, "ds3231: FAIL");

    rtc.year = 2026;
    rtc.month = 6;
    rtc.day = 23;
    rtc.hour = 18;
    rtc.minute = 21;
    rtc.second = 0;
    ds3231_set_time(&rtc);

    while (1) {
        ds3231_get_time(&rtc);
        sht3x_read(&sht3x, NULL, NULL);
        ds18b20_update(&ds18b20, NULL);

        GET_TICK(now);

        uint32_t sht3x_data_age_ms = (now - sht3x.last_update) / US_TO_TICKS(1) / 1000;
        uint32_t ds18_temp_age_ms = (now - ds18b20.last_temp_update) / US_TO_TICKS(1) / 1000;

        snprintf(buf, sizeof(buf), "%u.%02u.%02u %02u:%02u:%02u\n\nSHT31 %d.%dC %u%%\n%lu ms ago\n\nDS18B20 %d.%dC\n%lu ms ago\n",
                 rtc.year, rtc.month, rtc.day, rtc.hour, rtc.minute, rtc.second,
                 sht3x.temp_x10 / 10, abs(sht3x.temp_x10 % 10), sht3x.hum, sht3x_data_age_ms,
                 ds18b20.temp_x10 / 10, abs(ds18b20.temp_x10 % 10), ds18_temp_age_ms);

        ssd1306_print(&oled, buf);
    }
}
```

***AVR bare metal (AVR‑libc, no Arduino)***
```c
#include "ds18b20.h"
#include "sht3x.h"
#include "ssd1306.h"
#include "ds3231.h"
#include <stdio.h>

int main(void)
{
    /* AVR bare metal pin format: PIN_INIT(Pxn) */
    gpio_pin_t sda = PIN_INIT(PC4);
    gpio_pin_t scl = PIN_INIT(PC5);
    gpio_pin_t ow_pin = PIN_INIT(PB0);

    ds18b20_t ds18b20;
    sht3x_t sht3x;
    ssd1306_t oled;
    ds3231_t rtc;

    char buf[128];
    uint32_t now;

    if (ssd1306_init(&oled, sda, scl, true, false))
        ssd1306_print(&oled, "Initializing...");

    if (ds18b20_init(&ds18b20, ow_pin))
        ssd1306_print(&oled, "ds18b20: OK");
    else
        ssd1306_print(&oled, "ds18b20: FAIL");

    if (sht3x_init(&sht3x, sda, scl, true))
        ssd1306_print(&oled, "sht3x: OK");
    else
        ssd1306_print(&oled, "sht3x: FAIL");

    if (ds3231_init(&rtc, sda, scl))
        ssd1306_print(&oled, "ds3231: OK");
    else
        ssd1306_print(&oled, "ds3231: FAIL");

    rtc.year = 2026;
    rtc.month = 6;
    rtc.day = 23;
    rtc.hour = 18;
    rtc.minute = 21;
    rtc.second = 0;
    ds3231_set_time(&rtc);

    while (1) {
        ds3231_get_time(&rtc);
        sht3x_read(&sht3x, NULL, NULL);
        ds18b20_update(&ds18b20, NULL);

        GET_TICK(now);

        uint32_t sht3x_data_age_ms = (now - sht3x.last_update) / US_TO_TICKS(1) / 1000;
        uint32_t ds18_temp_age_ms = (now - ds18b20.last_temp_update) / US_TO_TICKS(1) / 1000;

        snprintf(buf, sizeof(buf), "%u.%02u.%02u %02u:%02u:%02u\n\nSHT31 %d.%dC %u%%\n%lu ms ago\n\nDS18B20 %d.%dC\n%lu ms ago\n",
                 rtc.year, rtc.month, rtc.day, rtc.hour, rtc.minute, rtc.second,
                 sht3x.temp_x10 / 10, abs(sht3x.temp_x10 % 10), sht3x.hum, sht3x_data_age_ms,
                 ds18b20.temp_x10 / 10, abs(ds18b20.temp_x10 % 10), ds18_temp_age_ms);

        ssd1306_print(&oled, buf);
    }
}
```

***AVR with ARDUINO***
```c
#include <Arduino.h>
#include "ds18b20.h"
#include "sht3x.h"
#include "ssd1306.h"
#include "ds3231.h"

gpio_pin_t sda = PIN_INIT(PIN_A0);
gpio_pin_t scl = PIN_INIT(PIN_A1);
gpio_pin_t ow_pin = PIN_INIT(8);

ds18b20_t ds18b20;
sht3x_t sht3x;
ssd1306_t oled;
ds3231_t rtc;

char buf[128];
uint32_t now;

void setup()
{
    Serial.begin(115200);

    if (ssd1306_init(&oled, sda, scl, true, false))
        ssd1306_print(&oled, "Initializing...");

    if (ds18b20_init(&ds18b20, ow_pin))
        ssd1306_print(&oled, "ds18b20: OK");
    else
        ssd1306_print(&oled, "ds18b20: FAIL");

    if (sht3x_init(&sht3x, sda, scl, true)) 
        ssd1306_print(&oled, "sht3x: OK");
    else
        ssd1306_print(&oled, "sht3x: FAIL!");

    if (ds3231_init(&rtc, sda, scl))
        ssd1306_print(&oled, "ds3231: OK");
    else
        ssd1306_print(&oled, "ds3231: FAIL");

    rtc.year = 2026;
    rtc.month = 6;
    rtc.day = 23;
    rtc.hour = 18;
    rtc.minute = 21;
    rtc.second = 0;
    ds3231_set_time(&rtc);
}

void loop()
{
    ds3231_get_time(&rtc);
    sht3x_read(&sht3x, NULL, NULL);
    ds18b20_update(&ds18b20, NULL);

    GET_TICK(now);
    
    uint32_t sht3x_data_age_ms = (now - sht3x.last_update) / US_TO_TICKS(1) / 1000;
    uint32_t ds18_temp_age_ms = (now - ds18b20.last_temp_update) / US_TO_TICKS(1) / 1000;

    snprintf(buf, sizeof(buf), "%u.%02u.%02u %02u:%02u:%02u\n\nSHT31 %d.%dC %u\%%\n%lu ms ago\n\nDS18B20 %d.%dC\n%lu ms ago\n",
             rtc.year, rtc.month, rtc.day, rtc.hour, rtc.minute, rtc.second,
             sht3x.temp_x10 / 10, abs(sht3x.temp_x10 % 10), sht3x.hum, sht3x_data_age_ms,
             ds18b20.temp_x10 / 10, abs(ds18b20.temp_x10 % 10), ds18_temp_age_ms);

    ssd1306_print(&oled, buf);
}

```

## DS18B20 wiring diagram 
![DS18B20 wiring diagram](DS18B20_wiring.svg)