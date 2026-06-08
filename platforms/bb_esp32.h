/**
 * @file bb_esp32.h
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

#define gpio_pin_t uint32_t

#define BB_GPIO_PIN_HIGH_Z(PIN)            \
    do                                     \
    {                                      \
        GPIO.enable_w1tc = (1UL << (PIN)); \
        GPIO.out_w1ts = (1UL << (PIN));    \
        GPIO.pin[(PIN)].pad_driver = 0;    \
    } while (0)

#define BB_GPIO_PIN_PULL_DOWN(PIN)      \
    do                                  \
    {                                   \
        GPIO.out_w1tc = (1UL << (PIN)); \
    } while (0)

#define BB_GPIO_PIN_PULL_UP(PIN)           \
    do                                     \
    {                                      \
        GPIO.out_w1ts = (1UL << (PIN));    \
        GPIO.enable_w1ts = (1UL << (PIN)); \
        GPIO.pin[(PIN)].pad_driver = 1;    \
    } while (0)

#define BB_GPIO_PIN_READ(PIN) ((GPIO.in >> (PIN)) & 1)

#define BB_GPIO_PIN_INIT(PIN)                                    \
    do                                                           \
    {                                                            \
        PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[(PIN)], PIN_FUNC_GPIO); \
        gpio_pulldown_dis(PIN);                                  \
        gpio_pullup_dis(PIN);                                    \
        GPIO.pin[(PIN)].func = 0;                                \
        BB_GPIO_PIN_HIGH_Z(PIN);                                 \
    } while (0)

#ifdef __XTENSA__
/* 234 - CCOUNT Xtensa special reg */
#define BB_GET_TICKS(TICKS) asm volatile("rsr %0, %1" : "=r"(TICKS) : "i"(234))
#endif /*__XTENSA__*/

#ifdef __riscv
#define BB_GET_TICKS(TICKS) asm volatile("rdcycle %0" : "=r"(TICKS))
#endif /*__riscv*/

#define BB_TICKS_SOURCE_INIT() \
    {                          \
    }

#define BB_US_TO_TICKS(US) ((uint32_t)(US) * (esp_clk_cpu_freq() / 1000000UL))

#endif /* ESP_PLATFORM */

#ifdef __cplusplus
}
#endif
