/**
 * @file bb_stm32f1.h
 * @brief STM32F1xx specific GPIO and timing functions for the longwires library.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifdef STM32F1

#include <stm32f1xx.h>

/**
 * @brief GPIO pin descriptor
 * @note Usage:
 * @note gpio_pin_t sda = STM32_PIN(GPIOB, 6);
*/
typedef struct gpio_pin_t
{
    GPIO_TypeDef      *GPIOx;              /**< Pointer to GPIO port registers       */
    volatile uint32_t *cr_reg;             /**< Pointer to CRL or CRH for this pin   */
    uint32_t           cr_mask_clear;      /**< Mask to clear the 4-bit field        */
    uint32_t           cr_bits_open_drain; /**< MODE + CNF for Open Drain            */
    uint32_t           cr_bits_push_pull;  /**< MODE + CNF for Push‑Pull             */
    uint32_t           bsrr_high;          /**< BSRR value for set (HIGH)            */
    uint32_t           bsrr_low;           /**< BSRR value for reset (LOW)           */
    uint32_t           pin;                /**< Pin number within port (0..15)        */
} gpio_pin_t;


/** @brief Calculate port index for clock enable */
#define BB_PORT_INDEX(GPIOx) \
    (((uint32_t)(GPIOx) - GPIOA_BASE) / (GPIOB_BASE - GPIOA_BASE))

/** @brief Enable APB2 clock for GPIO port */
#define BB_GPIO_ENABLE_CLOCK(GPIOx) \
    (RCC->APB2ENR |= (1UL << (2UL + BB_PORT_INDEX(GPIOx))))

/**
 * @brief Pin initialiser 
 * @param GPIOx GPIO port: GPIOA, GPIOB, ...
 * @param pin pin number 
 * @return configured and initialized pin in HI-Z state
 * @note Usage:
 * @note gpio_pin_t sda = STM32_PIN(GPIOB, 6);
*/
static inline gpio_pin_t STM32_PIN(GPIO_TypeDef *GPIOx, uint32_t pin)
{
    gpio_pin_t p;
    uint32_t shift = (pin < 8UL) ? (pin * 4UL) : ((pin - 8UL) * 4UL);

    p.GPIOx             = GPIOx;
    p.cr_reg            = (pin < 8UL) ? &GPIOx->CRL : &GPIOx->CRH;
    p.cr_mask_clear     = 0xFUL << shift;
    p.cr_bits_open_drain = (0x2UL | (0x1UL << 2)) << shift;  /* MODE=10, CNF=01 */
    p.cr_bits_push_pull  = (0x2UL | (0x0UL << 2)) << shift;  /* MODE=10, CNF=00 */
    p.bsrr_high         = 1UL << pin;
    p.bsrr_low          = 1UL << (pin + 16UL);
    p.pin               = pin;

    /* Enable clock and configure Open Drain, start Hi‑Z */
    BB_GPIO_ENABLE_CLOCK(GPIOx);
    *p.cr_reg = (*p.cr_reg & ~p.cr_mask_clear) | p.cr_bits_open_drain;
    GPIOx->BSRR = p.bsrr_high;

    return p;
}


/**
 * @brief High-impedance state (Open Drain, output released).
 * @param PIN gpio_pin_t structure
 */
#define BB_GPIO_PIN_HIGH_Z(PIN)                                              \
    do                                                                       \
    {                                                                        \
        *(PIN).cr_reg = (*(PIN).cr_reg & ~(PIN).cr_mask_clear) | (PIN).cr_bits_open_drain; \
        (PIN).GPIOx->BSRR = (PIN).bsrr_high;                                  \
    } while (0)

/**
 * @brief Strong pull-down (Open Drain, output driven LOW).
 * @param PIN gpio_pin_t structure
 */
#define BB_GPIO_PIN_PULL_DOWN(PIN)                                           \
    do                                                                       \
    {                                                                        \
        *(PIN).cr_reg = (*(PIN).cr_reg & ~(PIN).cr_mask_clear) | (PIN).cr_bits_open_drain; \
        (PIN).GPIOx->BSRR = (PIN).bsrr_low;                                   \
    } while (0)

/**
 * @brief Strong pull-up (Push‑Pull, output driven HIGH).
 * @param PIN gpio_pin_t structure
 */
#define BB_GPIO_PIN_PULL_UP(PIN)                                             \
    do                                                                       \
    {                                                                        \
        *(PIN).cr_reg = (*(PIN).cr_reg & ~(PIN).cr_mask_clear) | (PIN).cr_bits_push_pull; \
        (PIN).GPIOx->BSRR = (PIN).bsrr_high;                                  \
    } while (0)

/**
 * @brief Read current logic level on the pin.
 * @param PIN gpio_pin_t structure
 * @return 0 or 1
 */
#define BB_GPIO_PIN_READ(PIN) \
    (((PIN).GPIOx->IDR >> (PIN).pin) & 1UL)

#define BB_GPIO_PIN_INIT(PIN) {}


/** @brief Initialise DWT cycle counter */
#define BB_TICKS_SOURCE_INIT()                       \
    do                                               \
    {                                                \
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; \
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;         \
    } while (0)

/** @brief Read current tick count */
#define BB_GET_TICKS(TICKS) (TICKS = DWT->CYCCNT)

/** @brief Convert microseconds to ticks */
#define BB_US_TO_TICKS(US) ((uint32_t)(US) * (SystemCoreClock / 1000000UL))



#endif /* STM32F1 */

#ifdef __cplusplus
}
#endif