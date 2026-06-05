/**
 * @file bb_avr.h
 * @brief Platform abstraction for AVR (bare metal, no Arduino)
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef AVR

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdbool.h>

    /**
     * @brief AVR gpio pin descriptor
     * @param port PORTx pointer: &PORTB, &PORTC, etc...
     * @param bit pin bit: PB0, PB1, PB2, etc...
     * @example gpio_pin_t my_pin = {&PORTB, PB0};
     */
    typedef struct
    {
        volatile uint8_t *port; /* &PORTx */
        uint8_t bit;            /* 0-7 */
    } gpio_pin_t;

#define BB_GPIO_PIN_HIGH_Z(PIN) (*(PIN.port - 1) &= ~(1 << PIN.bit))
#define BB_GPIO_PIN_PULL_DOWN(PIN)           \
    do                                       \
    {                                        \
        (*PIN.port &= ~(1 << PIN.bit));      \
        (*(PIN.port - 1) |= (1 << PIN.bit)); \
    } while (0)
#define BB_GPIO_PIN_READ(PIN) ((*(PIN.port - 2) >> PIN.bit) & 1)
#define BB_GPIO_INIT_OPEN_DRAIN(PIN) BB_GPIO_PIN_HIGH_Z(PIN)

    void BB_TICKS_SOURCE_INIT(void);
    extern volatile uint32_t _bb_timer1_overflow;

#define BB_GET_TICKS(TICKS) (TICKS = (_bb_timer1_overflow + TCNT1))
#define BB_US_TO_TICKS(US) ((uint32_t)(US) * (F_CPU / 1000000UL))
#define BB_DELAY_TICKS(TICKS)                                                     \
    do                                                                            \
    {                                                                             \
        uint32_t _bb_delay_start_tick = _bb_timer1_overflow + TCNT1;              \
        do                                                                        \
        {                                                                         \
                                                                                  \
        } while (((_bb_timer1_overflow + TCNT1) - _bb_delay_start_tick) < TICKS); \
    } while (0)

#endif /* AVR */

#ifdef __cplusplus
}
#endif