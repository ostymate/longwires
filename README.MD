
# longwires

**Software bitbang I2C and 1-Wire library for long-distance wiring**


# Features

- Pure software bitbang – No hardware I2C/1-Wire peripheral needed
- Wide I2C frequency range – 1 Hz to 1 MHz+ 
- Long distance wiring (tested 100 meters twisted pair)
- Clock stretching support – Handles slow slave devices with configurable timeout
- Non-blocking DS18B20 – State machine based, doesn't block your main loop
- Fixed-point data – No floating point overhead (optional float support for SHT3X)
- Platform abstraction – Clean separation between core logic and hardware

## Adaptive Timing

The library measures actual SDA/SCL rise time on every transaction
and adjusts frequency automatically. This means:

* **Any pull-up resistors work** — 1 kΩ, 4.7 kΩ, 10 kΩ, mismatched
  between SDA and SCL, or even the weak internal pull-ups inside
  the sensor (typically ~50 kΩ). No code changes needed.

* **Asymmetric lines are fine** — different pull-up values on SDA
  and SCL, different wire lengths. The worst-case rise time
  determines the safe frequency for both.

* **No master-side pull-ups required** — if your sensor already has
  pull-ups on its board, just connect directly.
  The library adapts.

* **Hot-plug safe** — pull-ups can be changed during operation.
  The library recovers on the next transaction with updated timing.

* **User-defined ceiling** — `freq_hz` sets the maximum frequency.
  The library will never exceed it, but will drop lower if the
  physical line demands it (long wires, weak pull-ups, high capacitance).

The result: you stop thinking about I2C bus timing and just wire
things up. The library figures out the rest.

# Supported MCU
    - ESP32 (Xtensa and RISC-V based) 
    - STM32F1 
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
***ESP32 with ESP-IDF framework***
```C
#include "sht3x.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

gpio_pin_t SDA = 25;
gpio_pin_t SCL = 26;

void app_main(void)
{
    sht3x_sensor_t sensor;
    float humidity, temperature;
    sht3x_init(&sensor, SDA, SCL, true);

    while (1)
    {
        sht3x_read(&sensor, &temperature, &humidity);
        printf("Temperature: %.1f C, Humidity: %.1f%%\n", temperature, humidity);
        printf("Success count: %lu, Error count: %lu\n", sensor.success_count, sensor.error_count);
        printf("Sensor is %s\n", sensor.is_active ? "active" : "inactive");
        printf("actual_freq_hz: %lu\n", sensor.i2c_bb_device.actual_freq_hz); // Print the actual I2C frequency for debugging
        vTaskDelay(pdMS_TO_TICKS(1000));

    }
}

```  

***STM32F1 PlatformIO with cmsis framework***
```C

#include "sht3x.h"
#include "stdio.h"

gpio_pin_t SDA = {.GPIOx = GPIOB, .pin = 6};
gpio_pin_t SCL = {.GPIOx = GPIOB, .pin = 7};

sht3x_sensor_t sensor;

int main(void)
{

    sht3x_init(&sensor, SDA, SCL, true); // true - set default sht3x addr (0x44), false - set alt addr (0x45)

    while (1)
    {
        sht3x_read(&sensor, 0, 0);
        printf("temp_x10: %d, hum: %d, is_active: %d\n", sensor.temp_x10, sensor.hum, sensor.is_active);
        printf("I2C Read Speed: %lu bps, I2C Write Speed: %lu bps\n", sensor.i2c_bb_device.bits_per_second_read, sensor.i2c_bb_device.bits_per_second_write);
        printf("Total Errors: %lu, Total Successes: %lu\n", sensor.error_count, sensor.success_count);
        printf("rise time: %lu us, hold ticks: %lu\n", sensor.i2c_bb_device.t_rise_us, sensor.i2c_bb_device.t_hold_ticks);
        printf("actual freq: %lu Hz, target freq: %lu Hz\n", sensor.i2c_bb_device.actual_freq_hz, sensor.i2c_bb_device.freq_hz);
        BB_DELAY_US(1000000000UL); // 1 second
    }
}


```



## why?
because hardware I2C can't handle 100 meters of twisted pair or communicate with I2C sensors with adaptive frequency