# LongWires

**Software bitbang I2C and 1-Wire library for ESP32 and STM32F1**

---

## ✨ Features

- **Pure software bitbang** – No hardware I2C/1-Wire peripheral needed
- **Wide I2C frequency range** – 1 Hz to 1 MHz+ (tested @ 240 MHz ESP32)
- **Clock stretching support** – Handles slow slave devices with configurable timeout
- **Non-blocking DS18B20** – State machine based, doesn't block your main loop
- **Fixed-point data** – No floating point overhead (optional float support for SHT3X)
- **Platform abstraction** – Clean separation between core logic and hardware



## 🎯 Supported Platforms

 **ESP32** (Xtensa and RISC-V) 
 **STM32F1** 


## 📦 Included Drivers

*I2C devices:*
**DS3231** [Link](https://datasheets.maximintegrated.com/en/ds/DS3231.pdf) 
**SHT3X** [Link](https://sensirion.com/media/documents/213E6A3B/63A5A569/Datasheet_SHT3x_DIS.pdf) 

*1-Wire devices:*
**DS18B20** [Link](https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf)




## 🔧 Installation

### ESP-IDF

**idf_component.yml:**
dependencies:
  ostymate/longwires:
    version: "1.0.1"
    git: https://github.com/ostymate/longwires.git


### Platformio 
**platformio.ini:**
lib_deps = 
    https://github.com/ostymate/longwires.git