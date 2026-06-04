/**
 * @file bb_avr.h
 * @brief Platform abstraction for AVR (bare metal, no Arduino)
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifdef AVR

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdbool.h>

#define gpio_pin_t uint8_t


#ifndef BB_AVR_PORT_INDEX
#define BB_AVR_PORT_INDEX(PIN) ((PIN) / 8)
#endif

#ifndef BB_AVR_BIT
#define BB_AVR_BIT(PIN) ((PIN) & 7)
#endif

#define _BB_AVR_PORT0_BASE _SFR_MEM_ADDR(PINB)  // memory address, not I/O!
#define _BB_AVR_PORT_OFFSET(PIN) ((uint8_t)(BB_AVR_PORT_INDEX(PIN) * 3))

#define _BB_AVR_PIN_ADDR(PIN)   (_BB_AVR_PORT0_BASE + _BB_AVR_PORT_OFFSET(PIN))
#define _BB_AVR_DDR_ADDR(PIN)   (_BB_AVR_PORT0_BASE + _BB_AVR_PORT_OFFSET(PIN) + 1)
#define _BB_AVR_PORT_ADDR(PIN)  (_BB_AVR_PORT0_BASE + _BB_AVR_PORT_OFFSET(PIN) + 2)

#define _BB_AVR_SFR(ADDR) (*(volatile uint8_t *)(ADDR))

static inline uint8_t bb_gpio_init_open_drain(gpio_pin_t pin)
{
    _BB_AVR_SFR(_BB_AVR_PORT_ADDR(pin)) &= ~(1 << BB_AVR_BIT(pin));
    _BB_AVR_SFR(_BB_AVR_DDR_ADDR(pin))  &= ~(1 << BB_AVR_BIT(pin));
    return 0;
}

static inline uint8_t bb_gpio_init_push_pull(gpio_pin_t pin)
{
    _BB_AVR_SFR(_BB_AVR_DDR_ADDR(pin)) |= (1 << BB_AVR_BIT(pin));
    return 0;
}

static inline uint8_t bb_gpio_init_input_floating(gpio_pin_t pin)
{
    _BB_AVR_SFR(_BB_AVR_DDR_ADDR(pin))  &= ~(1 << BB_AVR_BIT(pin));
    _BB_AVR_SFR(_BB_AVR_PORT_ADDR(pin)) &= ~(1 << BB_AVR_BIT(pin));
    return 0;
}

static inline uint8_t bb_gpio_pin_set(gpio_pin_t pin)
{
    _BB_AVR_SFR(_BB_AVR_DDR_ADDR(pin)) &= ~(1 << BB_AVR_BIT(pin));
    return 0;
}

static inline uint8_t bb_gpio_pin_reset(gpio_pin_t pin)
{
    _BB_AVR_SFR(_BB_AVR_PORT_ADDR(pin)) &= ~(1 << BB_AVR_BIT(pin));
    _BB_AVR_SFR(_BB_AVR_DDR_ADDR(pin))  |=  (1 << BB_AVR_BIT(pin));
    return 0;
}

static inline uint8_t bb_gpio_pin_read(gpio_pin_t pin)
{
    return (_BB_AVR_SFR(_BB_AVR_PIN_ADDR(pin)) >> BB_AVR_BIT(pin)) & 1;
}

#define BB_GPIO_INIT_OPEN_DRAIN(PIN)     bb_gpio_init_open_drain(PIN)
#define BB_GPIO_INIT_PUSH_PULL(PIN)      bb_gpio_init_push_pull(PIN)
#define BB_GPIO_INIT_INPUT_FLOATING(PIN) bb_gpio_init_input_floating(PIN)
#define BB_GPIO_PIN_SET(PIN)             bb_gpio_pin_set(PIN)
#define BB_GPIO_PIN_RESET(PIN)           bb_gpio_pin_reset(PIN)
#define BB_GPIO_PIN_READ(PIN)            bb_gpio_pin_read(PIN)

void bb_avr_timer_init(void);

#define BB_TICKS_SOURCE_INIT() bb_avr_timer_init()

extern volatile uint32_t _bb_timer1_overflow;

static inline uint32_t bb_avr_ticks_get(void)
{
    uint32_t ovf = _bb_timer1_overflow;
    uint16_t cnt = TCNT1;
    return ovf + cnt;
}

#define BB_GET_TICKS(TICKS) do { (TICKS) = bb_avr_ticks_get(); } while(0)

#define CPU_CLOCK_FREQ_HZ F_CPU

#define BB_TICKS_PER_US       (F_CPU / 1000000UL)
#define BB_US_TO_TICKS(US)    ((uint32_t)(US) * BB_TICKS_PER_US)
#define BB_TICKS_TO_US(TICKS) ((uint32_t)(TICKS) / BB_TICKS_PER_US)

#define BB_DELAY_US(US) _delay_us(US)

static inline void bb_delay_ticks_avr(uint32_t ticks)
{
    uint32_t start = bb_avr_ticks_get();
    while ((bb_avr_ticks_get() - start) < ticks) {
        /* spin */
    }
}

#define BB_DELAY_TICKS(TICKS) bb_delay_ticks_avr(TICKS)

#endif /* AVR */

#ifdef __cplusplus
}
#endif