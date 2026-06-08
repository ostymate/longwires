/**
 * @file bb_stm32f1.h
 * @brief STM32F1xx specific GPIO and timing functions for the longwires library. 
 * This header defines the GPIO pin structure and functions for configuring and manipulating GPIO pins on STM32F1xx microcontrollers, 
 * as well as timing functions for adaptive I2C communication. 
 * The implementation ensures that the longwires library can perform bit-banging I2C communication reliably on STM32F1xx devices,
 * even with long wires and slow peripherals, by measuring rise times and adjusting hold times accordingly. 
 * This file is essential for enabling the longwires library to work effectively on STM32F1xx platforms, 
 * providing the necessary hardware abstraction for GPIO control and timing management.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifdef STM32F1

#include <stm32f1xx.h>

/**
 * @brief GPIO pin descriptor for STM32.
 * @param GPIOx GPIO port. GPIOA, GPIOB, etc...
 * @param pin pin number 0..15
 * @example: gpio_pin_t sda = {GPIOB, 6};
 */
typedef struct gpio_pin_t
{
    GPIO_TypeDef *GPIOx; 
    uint32_t pin;        
} gpio_pin_t;

#define BB_GPIO_MODE_OUTPUT_2MHZ 0x2UL // Output mode, max SPEED 2 MHz
#define BB_GPIO_CNF_OUTPUT_OPEN_DRAIN 0x1UL // General purpose output open-drain

/* Calculate port index */
#define BB_PORT_INDEX(GPIOx) (((uint32_t)GPIOx - GPIOA_BASE) / (GPIOB_BASE - GPIOA_BASE))

/* Enable clock for specific GPIO port */
#define BB_GPIO_ENABLE_CLOCK(GPIOx) (RCC->APB2ENR |= (1UL << (2UL + BB_PORT_INDEX(GPIOx))))

static inline void bb_gpio_configure_pin(GPIO_TypeDef *GPIOx, uint32_t pin, uint32_t mode, uint32_t cnf)
{
    BB_GPIO_ENABLE_CLOCK(GPIOx);

    uint32_t shift = pin < 8UL ? pin * 4UL : (pin - 8UL) * 4UL;
    uint32_t mask = 0xFUL << shift;
    uint32_t value = ((cnf << 2) | mode) << shift;

    if (pin < 8UL)
        GPIOx->CRL = (GPIOx->CRL & ~mask) | value;
    else
        GPIOx->CRH = (GPIOx->CRH & ~mask) | value;
}

#define BB_GPIO_PIN_INIT(PIN) bb_gpio_configure_pin(PIN.GPIOx, PIN.pin, BB_GPIO_MODE_OUTPUT_2MHZ, BB_GPIO_CNF_OUTPUT_OPEN_DRAIN)
#define BB_GPIO_PIN_HIGH_Z(PIN) (((GPIO_TypeDef *)PIN.GPIOx)->BSRR = (1UL << (PIN.pin)))
#define BB_GPIO_PIN_PULL_DOWN(PIN) (((GPIO_TypeDef *)PIN.GPIOx)->BSRR = (1UL << ((PIN.pin) + 16UL)))
#define BB_GPIO_PIN_READ(PIN) ((((GPIO_TypeDef *)PIN.GPIOx)->IDR >> (PIN.pin)) & 1UL)

#define BB_TICKS_SOURCE_INIT()                      \
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; \
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

#define BB_GET_TICKS(TICKS) (TICKS = DWT->CYCCNT)

#define BB_US_TO_TICKS(US) ((uint32_t)(US) * (SystemCoreClock / 1000000UL))

#endif /* STM32F1 */

#ifdef __cplusplus
}
#endif