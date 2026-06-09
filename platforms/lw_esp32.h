/**
 * @file lw_esp32.h
 * @brief platform dependent components of software I2C and onewire for ESP32 (Xtensa and RISC-V series)
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef ESP_PLATFORM

#include <driver/gpio.h>
#include <esp32/rom/gpio.h>
#include <esp_private/esp_clk.h>

    /**
     * @brief GPIO pin descriptor
     * @note usage:
     * @note gpio_pin_t my_pin = ESP32_PIN(21);
     */
    typedef uint32_t gpio_pin_t;

/**
 * @brief High-impedance state
 * @param PIN gpio_pin_t
 */
#define PIN_Z(PIN)                                                     \
    do                                                                 \
    {                                                                  \
        (*(volatile uint32_t *)GPIO_ENABLE_W1TC_REG) = (1UL << (PIN)); \
        (*(volatile uint32_t *)GPIO_OUT_W1TS_REG) = (1UL << (PIN));    \
    } while (0)

/**
 * @brief Strong pull-down: output enabled, line driven LOW
 * @param PIN gpio_pin_t (pin number)
 */
#define PIN_LOW(PIN) ((*(volatile uint32_t *)GPIO_OUT_W1TC_REG) = (1UL << (PIN)))

/**
 * @brief Strong pull-up: output enabled, line driven HIGH
 * @param PIN gpio_pin_t (pin number)
 */
#define PIN_HIGH(PIN)                                                  \
    do                                                                 \
    {                                                                  \
        (*(volatile uint32_t *)GPIO_ENABLE_W1TS_REG) = (1UL << (PIN)); \
        (*(volatile uint32_t *)GPIO_OUT_W1TS_REG) = (1UL << (PIN));    \
    } while (0)

/**
 * @brief Read current logic level on the pin
 * @param PIN gpio_pin_t (pin number)
 * @return 0 or 1
 */
#define PIN_READ(PIN) ((*(volatile uint32_t *)GPIO_IN_REG >> (PIN)) & 1UL)

    /**
     * @brief Initialise pin
     * @param pin pin number
     * @return initialized pin
     * @note usage:
     * @note gpio_pin_t my_pin = ESP32_PIN(21);
     */
    static inline gpio_pin_t ESP32_PIN(uint32_t pin)
    {
        gpio_config_t _conf = {
            .pin_bit_mask = (1ULL << (pin)),
            .mode = GPIO_MODE_INPUT_OUTPUT_OD,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&_conf);
        PIN_Z(pin);
        return pin;
    }

/**
 * @brief initialize ticks source
 * @note isn't required for ESP32
 */
#define TICK_INIT() \
    do              \
    {               \
    } while (0)

#ifdef __XTENSA__
/**
 * @brief get current CPU tick
 * @note usage:
 * @note uint32_t current_tick;
 * @note GET_TICK(current_tick);
 */
#define GET_TICK(TICK) asm volatile("rsr %0, %1" : "=r"(TICK) : "i"(234))
#endif

#ifdef __riscv
/**
 * @brief get current CPU tick
 * @param TICK tick value
 * @note usage:
 * @note uint32_t current_tick;
 * @note GET_TICK(current_tick);
 */
#define GET_TICK(TICK) asm volatile("rdcycle %0" : "=r"(TICK))
#endif

/**
 * @brief convert microseconds to ticks
 * @param US microseconds value
 * @return ticks value
 */
#define US_TO_TICKS(US) ((uint32_t)(US) * (esp_clk_cpu_freq() / 1000000UL))

#endif /* ESP_PLATFORM */

#ifdef __cplusplus
}
#endif