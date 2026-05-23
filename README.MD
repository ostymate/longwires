
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
gpio_pin_t VCC = 15;


void app_main(void)
{
    sht3x_sensor_t sensor;
    sht3x_init(&sensor, SDA, SCL, 100000, 0x44, &VCC);

    while (1)
    {
        sht3x_perform_measurement(&sensor, 0, 0);
        printf("Temperature: %.1f C, Humidity: %d%%\n", sensor.temp_x10 / 10.0, sensor.hum);
        printf("Success count: %lu, Error count: %lu\n", sensor.success_count, sensor.error_count);
        printf("Sensor is %s\n", sensor.is_active ? "active" : "inactive");
        printf("actual_freq_hz: %lu\n", sensor.i2c_bb_device.actual_freq_hz); // Print the actual I2C frequency for debugging
        vTaskDelay(pdMS_TO_TICKS(1000));

    }
}

```  

***STM32F1 PlatformIO***
```C

```



## why?
because hardware I2C can't handle 100 meters of twisted pair or communicate with I2C sensors with adaptive frequency