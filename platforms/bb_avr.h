/**
 * @file bb_avr.h
 * @brief Platform abstraction for AVR — bare metal / arduino GPIO and timing
 * @note requirements: TIMER1
 *
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
     * @brief GPIO pin descriptor
     * @param DDRx Data Direction Register  (&DDRx)
     * @param PORTx Port Output Register    (&PORTx)
     * @param PINx Port Input Register      (&PINx)
     * @param bit Pin number within port    (0..7)
     *
     * @note examples for easy init:
     * @note gpio_pin_t led_pin = AVR_PIN(PB5); <--- any AVR
     * @note gpio_pin_t led_pin = ARDUINO_PIN(13); <--- only with ARDUINO framework
     */
    typedef struct
    {
        volatile uint8_t *DDRx;
        volatile uint8_t *PORTx;
        volatile uint8_t *PINx;
        uint8_t bit;
    } gpio_pin_t;

#ifndef DDRC
#ifdef DDRA
#define DDR_OFFSET (DDRB - DDRA)
#define PORT_OFFSET (PORTB - PORTA)
#define PIN_OFFSET (PINB - PINA)
#else
#define DDR_OFFSET 0
#define PORT_OFFSET 0
#define PIN_OFFSET 0
#endif /* DDRA */
#else
#define DDR_OFFSET (DDRC - DDRB)
#define PORT_OFFSET (PORTC - PORTB)
#define PIN_OFFSET (PINC - PINB)
#endif /* DDRC */

/**
 * @brief Initialise a gpio_pin_t from an AVR pin name.
 * @param pin  AVR pin macro: PB0, PC3, PD5, …
 * @return     Initialised gpio_pin_t structure
 */
#define AVR_PIN(pin)                                                \
    {                                                               \
        .DDRx = &DDRB + (((int)(#pin[1] - 'B')) * (DDR_OFFSET)),    \
        .PORTx = &PORTB + (((int)(#pin[1] - 'B')) * (PORT_OFFSET)), \
        .PINx = &PINB + (((int)(#pin[1] - 'B')) * (PIN_OFFSET)),    \
        .bit = pin}

#ifdef ARDUINO
#include <Arduino.h>
    /**
     * @brief Initialise a gpio_pin_t from an Arduino pin number.
     * @param pin  Arduino digital pin number (0..13 on Uno)
     * @return     Initialised gpio_pin_t structure
     */
    static inline gpio_pin_t ARDUINO_PIN(uint8_t pin)
    {
        gpio_pin_t p;
        p.DDRx = portModeRegister(digitalPinToPort(pin));
        p.PORTx = portOutputRegister(digitalPinToPort(pin));
        p.PINx = portInputRegister(digitalPinToPort(pin));
        p.bit = __builtin_ctz(digitalPinToBitMask(pin));
        return p;
    }
#endif /* ARDUINO */

/**
 * @brief High-impedance input: DDR=0, PORT=0
 * @param PIN gpio_pin_t structure
 */
#define BB_GPIO_PIN_HIGH_Z(PIN)          \
    do                                   \
    {                                    \
        *(PIN.DDRx) &= ~(1 << PIN.bit);  \
        *(PIN.PORTx) &= ~(1 << PIN.bit); \
    } while (0)

/**
 * @brief Strong pull-down (output low): DDR=1, PORT=0
 * @param PIN gpio_pin_t structure
 */
#define BB_GPIO_PIN_PULL_DOWN(PIN)       \
    do                                   \
    {                                    \
        *(PIN.PORTx) &= ~(1 << PIN.bit); \
        *(PIN.DDRx) |= (1 << PIN.bit);   \
    } while (0)

/**
 * @brief Strong pull-up (output high): DDR=1, PORT=1
 * @param PIN gpio_pin_t structure
 */
#define BB_GPIO_PIN_PULL_UP(PIN)        \
    do                                  \
    {                                   \
        *(PIN.PORTx) |= (1 << PIN.bit); \
        *(PIN.DDRx) |= (1 << PIN.bit);  \
    } while (0)

/**
 * @brief Read current logic level on the pin
 * @param PIN gpio_pin_t structure
 * @return 0 or 1
 */
#define BB_GPIO_PIN_READ(PIN) \
    ((*(PIN.PINx) >> PIN.bit) & 1)

/**
 * @brief Initialise pin for open-drain operation (same as Hi‑Z)
 * @param PIN gpio_pin_t structure
 */
#define BB_GPIO_PIN_INIT(PIN) BB_GPIO_PIN_HIGH_Z(PIN)

    /**
     * @brief Initialise Timer1 as a free-running tick source.
     * Configures Timer1 with prescaler 1 (each tick = 1 CPU cycle).
     * Overflow interrupt accumulates into a 32-bit counter.
     */
    void BB_TICKS_SOURCE_INIT(void);

    /**
     * @brief 32-bit tick counter, incremented by Timer1 overflow ISR.
     */
    extern volatile uint32_t _bb_timer1_overflow;

/**
 * @brief Capture current tick count (Timer1 + overflow counter).
 * @param TICKS Variable to store the result.
 */
#define BB_GET_TICKS(TICKS)                    \
    do                                         \
    {                                          \
        uint32_t _ovf;                         \
        uint16_t _tcnt;                        \
        do                                     \
        {                                      \
            _ovf = _bb_timer1_overflow;        \
            _tcnt = TCNT1;                     \
        } while (_ovf != _bb_timer1_overflow); \
        TICKS = _ovf + _tcnt;                  \
    } while (0)

#ifndef F_CPU
#error "F_CPU must be defined"
#endif

/**
 * @brief Convert microseconds to timer ticks.
 * @param US Microseconds.
 * @return Number of ticks.
 */
#define BB_US_TO_TICKS(US) ((uint32_t)(US) * (F_CPU / 1000000UL))

#endif /* AVR */

#ifdef __cplusplus
}
#endif