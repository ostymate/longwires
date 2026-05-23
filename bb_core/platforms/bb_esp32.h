/**
 * @file bb_esp32.h
 * @brief platform dependent components of software I2C and onewire for ESP32 (Xtensa and RISC-V series)
 */

#pragma once

#include <driver/gpio.h>
#include <esp32/rom/gpio.h>
#include <esp_private/esp_clk.h>

#define gpio_pin_t uint32_t

static inline void bb_gpio_configure_pin(uint32_t pin, gpio_mode_t mode)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = mode,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
}

#define BB_GPIO_INIT_OPEN_DRAIN(PIN) bb_gpio_configure_pin((PIN), GPIO_MODE_INPUT_OUTPUT_OD)
#define BB_GPIO_INIT_PUSH_PULL(PIN) bb_gpio_configure_pin((PIN), GPIO_MODE_INPUT_OUTPUT)
#define BB_GPIO_INIT_INPUT_FLOATING(PIN) bb_gpio_configure_pin((PIN), GPIO_MODE_INPUT)

#define BB_GPIO_PIN_SET(PIN) ((*(volatile uint32_t *)GPIO_OUT_W1TS_REG) = (1UL << (PIN)))
#define BB_GPIO_PIN_RESET(PIN) ((*(volatile uint32_t *)GPIO_OUT_W1TC_REG) = (1UL << (PIN)))
#define BB_GPIO_PIN_READ(PIN) ((*(volatile uint32_t *)GPIO_IN_REG >> (PIN)) & 1UL)

#ifdef __XTENSA__
/* 234 - CCOUNT Xtensa special reg */
#define BB_GET_TICKS(TICKS) asm volatile("rsr %0, %1" : "=r"(TICKS) : "i"(234))
#endif /*__XTENSA__*/

#ifdef __riscv
#define BB_GET_TICKS(TICKS) asm volatile("rdcycle %0" : "=r"(TICKS))
#endif /*__riscv*/

#define CPU_CLOCK_FREQ_HZ (esp_clk_cpu_freq())

#define BB_TICKS_SOURCE_INIT() {}