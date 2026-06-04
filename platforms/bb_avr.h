/**
 * @file bb_avr.h
 * @brief Platform abstraction for AVR (bare metal, no Arduino required but )
 */

#pragma once


#ifdef AVR

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define gpio_pin_t uint8_t

#ifndef BB_AVR_PORT_INDEX
#define BB_AVR_PORT_INDEX(PIN) ((PIN) / 8)
#endif

#ifndef BB_AVR_BIT
#define BB_AVR_BIT(PIN) ((PIN) & 7)
#endif

#define _BB_AVR_PORT0_BASE    _SFR_IO_ADDR(PORTB)
#define _BB_AVR_PORT_OFFSET(PIN) ((uint8_t)(BB_AVR_PORT_INDEX(PIN) * 3))

#define _BB_AVR_PIN_ADDR(PIN)  (_BB_AVR_PORT0_BASE + _BB_AVR_PORT_OFFSET(PIN))
#define _BB_AVR_DDR_ADDR(PIN)  (_BB_AVR_PORT0_BASE + _BB_AVR_PORT_OFFSET(PIN) + 1)
#define _BB_AVR_PORT_ADDR(PIN) (_BB_AVR_PORT0_BASE + _BB_AVR_PORT_OFFSET(PIN) + 2)

#define _BB_AVR_SFR(ADDR) (*(volatile uint8_t *)(ADDR))

static inline uint8_t bb_gpio_init_open_drain(gpio_pin_t pin)
{
    _BB_AVR_SFR(_BB_AVR_PORT_ADDR(pin)) &= ~(1 << BB_AVR_BIT(pin));
    _BB_AVR_SFR(_BB_AVR_DDR_ADDR(pin))  |=  (1 << BB_AVR_BIT(pin));
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


extern volatile uint16_t _bb_timer1_overflow;

void bb_avr_timer_init(void);

#define BB_TICKS_SOURCE_INIT() bb_avr_timer_init()

#define BB_GET_TICKS(TICKS) do {                       \
    uint32_t _ovf;                                     \
    uint16_t _cnt;                                     \
    uint8_t _sreg = SREG;                              \
    cli();                                             \
    _ovf = _bb_timer1_overflow;                        \
    _cnt = TCNT1;                                      \
    if ((TIFR1 & (1 << TOV1)) && _cnt < 0x8000)        \
        _ovf++;                                        \
    SREG = _sreg;                                      \
    (TICKS) = (_ovf << 16) | _cnt;                     \
} while(0)


#define CPU_CLOCK_FREQ_HZ F_CPU

#endif /* AVR */
