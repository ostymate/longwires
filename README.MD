
# longwires

**Software bitbang I2C and 1-Wire library for long-distance wiring**


# Features

- Pure software bitbang – No hardware I2C/1-Wire peripheral needed
- Automatic frequency adaptation – up to 100 kHz, slows down as the line demands
- Long distance wiring (tested 100 meters twisted pair)
- Clock stretching support – Handles slow slave devices with configurable timeout
- Non-blocking DS18B20 – State machine based, doesn't block your main loop
- Parasitic power DS18B20 – tested on 100 metres with two wires
- Fixed-point data – No floating point overhead (optional float support for SHT3X)
- Platform abstraction – separation between core logic and hardware



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
#include "sht3x.h"
#include "ds18b20.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void app_main(void)
{
    /* ESP32 pin format: PIN_INIT(gpio_number) */
    gpio_pin_t sda  = PIN_INIT(21);
    gpio_pin_t scl  = PIN_INIT(22);
    gpio_pin_t ow   = PIN_INIT(23);

    sht3x_sensor_t  sht;
    ds18b20_sensor_t ds18;

    sht3x_init(&sht, sda, scl, true);
    ds18b20_init(&ds18, ow);

    uint32_t start, now;
    GET_TICK(start);

    while (1) {
        ds18b20_update(&ds18);
        sht3x_read(&sht, 0, 0);
        GET_TICK(now);

        if (now - start > US_TO_TICKS(2000000)) {
            printf("SHT31: %d.%d C, %d %%RH\n",
                sht.temp_x10 / 10, sht.temp_x10 % 10, sht.hum);
            printf("DS18:  %d.%d C\n",
                ds18.temperature_x10 / 10, ds18.temperature_x10 % 10);
            start = now;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```  

***STM32F1***
```c
#include "sht3x.h"
#include "ds18b20.h"
#include <stdio.h>

int main(void)
{
    /* STM32 pin format: PIN_INIT(GPIOx, pin_number) */
    gpio_pin_t sda  = PIN_INIT(GPIOB, 6);
    gpio_pin_t scl  = PIN_INIT(GPIOB, 7);
    gpio_pin_t ow   = PIN_INIT(GPIOB, 8);

    sht3x_sensor_t  sht;
    ds18b20_sensor_t ds18;

    sht3x_init(&sht, sda, scl, true);
    ds18b20_init(&ds18, ow);

    uint32_t start, now;
    GET_TICK(start);

    while (1) {
        ds18b20_update(&ds18);
        sht3x_read(&sht, 0, 0);
        GET_TICK(now);

        if (now - start > US_TO_TICKS(2000000)) {
            printf("SHT31: %d.%d C, %d %%RH\n",
                sht.temp_x10 / 10, sht.temp_x10 % 10, sht.hum);
            printf("DS18:  %d.%d C\n",
                ds18.temperature_x10 / 10, ds18.temperature_x10 % 10);
            start = now;
        }
    }
}
```

***AVR bare metal (AVR‑libc, no Arduino)***
```c
#include "ds18b20.h"
#include <stdio.h>

int main(void)
{
    /* AVR bare metal pin format: PIN_INIT(Pxn) */
    gpio_pin_t ow = PIN_INIT(PB0);

    ds18b20_sensor_t ds18;
    ds18b20_init(&ds18, ow);

    uint32_t start, now;
    GET_TICK(start);

    while (1) {
        ds18b20_update(&ds18);
        GET_TICK(now);

        if (now - start > US_TO_TICKS(2000000)) {
            printf("DS18: %d.%d C\n",
                ds18.temperature_x10 / 10, ds18.temperature_x10 % 10);
            start = now;
        }
    }
}
```

***AVR with ARDUINO***
```c
#include "ds18b20.h"

gpio_pin_t ow = PIN_INIT(8);   /* Arduino pin format: PIN_INIT(digital_pin_number) */

ds18b20_sensor_t ds18;

void setup()
{
    Serial.begin(115200);
    ds18b20_init(&ds18, ow);
}

void loop()
{
    ds18b20_update(&ds18);

    Serial.print("DS18: ");
    Serial.print(ds18.temperature_x10 / 10);
    Serial.print(".");
    Serial.print(ds18.temperature_x10 % 10);
    Serial.println(" C");

    delay(2000);
}
```