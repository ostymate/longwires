
# longwires

**Software bitbang I2C and 1-Wire library for long-distance wiring**


# Features

- Pure software bitbang – No hardware I2C/1-Wire peripheral needed
- Wide I2C frequency range – 1 Hz to 1 MHz+ 
- Long distance wiring (I've tested 100 meters twisted pair)
- Clock stretching support – Handles slow slave devices with configurable timeout
- Non-blocking DS18B20 – State machine based, doesn't block your main loop
- Fixed-point data – No floating point overhead (optional float support for SHT3X)
- Platform abstraction – Clean separation between core logic and hardware

# Supported MCU:
    - ESP32 (Xtensa and RISC-V based) 
    - STM32F1 
# Supported frameworks:
    - ESP-IDF
    - Platformio
# Included drivers:
    I2C devices:
        SHT3X
        DS3231
    1-Wire:
        DS18B20

# how to install
**ESP-IDF**  
*idf_component.yml:*
```yaml
dependencies:
  longwires:
    git: https://github.com/ostymate/longwires.git
    version: "main"
```

**Platformio**  
*platformio.ini*: 
```ini
lib_deps = 
    https://github.com/ostymate/longwires.git
```