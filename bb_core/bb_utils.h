/**
 * @file bb_utils.h
 * @brief platform dependent components of software I2C and onewire for ESP32 (Xtensa and RISC-V series) and STM32F1
 */

#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef ESP_PLATFORM

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

/*ESP implementation without critical sections*/
#define BB_ENTER_CRITICAL() \
    {                       \
    }

#define BB_EXIT_CRITICAL() \
    {                      \
    }

#define BB_TICKS_SOURCE_INIT() \
    {                          \
    }

#endif /* ESP_PLATFORM */

#ifdef STM32F1
#include <stm32f1xx.h>

typedef struct gpio_pin_t
{
    GPIO_TypeDef *GPIOx;
    uint32_t pin;
} gpio_pin_t;

#define BB_GPIO_MODE_INPUT 0x0UL       // Input mode
#define BB_GPIO_MODE_OUTPUT_2MHZ 0x2UL // Output mode, max SPEED 2 MHz

#define BB_GPIO_CNF_INPUT_FLOATING 0x1UL    // Floating input
#define BB_GPIO_CNF_OUTPUT_PUSH_PULL 0x0UL  // General purpose output push-pull
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

#define BB_GPIO_INIT_OPEN_DRAIN(PIN) bb_gpio_configure_pin(PIN.GPIOx, PIN.pin, BB_GPIO_MODE_OUTPUT_2MHZ, BB_GPIO_CNF_OUTPUT_OPEN_DRAIN)
#define BB_GPIO_INIT_PUSH_PULL(PIN) bb_gpio_configure_pin(PIN.GPIOx, PIN.pin, BB_GPIO_MODE_OUTPUT_2MHZ, BB_GPIO_CNF_OUTPUT_PUSH_PULL)
#define BB_GPIO_INIT_INPUT_FLOATING(PIN) bb_gpio_configure_pin(PIN.GPIOx, PIN.pin, BB_GPIO_MODE_INPUT, BB_GPIO_CNF_INPUT_FLOATING)

#define BB_GPIO_PIN_SET(PIN) (((GPIO_TypeDef *)PIN.GPIOx)->BSRR = (1UL << (PIN.pin)))
#define BB_GPIO_PIN_RESET(PIN) (((GPIO_TypeDef *)PIN.GPIOx)->BSRR = (1UL << ((PIN.pin) + 16UL)))
#define BB_GPIO_PIN_READ(PIN) ((((GPIO_TypeDef *)PIN.GPIOx)->IDR >> (PIN.pin)) & 1UL)

#define BB_TICKS_SOURCE_INIT()                      \
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; \
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

#define BB_GET_TICKS(TICKS) (TICKS = DWT->CYCCNT)

#define CPU_CLOCK_FREQ_HZ (SystemCoreClock)

#define BB_ENTER_CRITICAL()             \
    uint32_t primask = __get_PRIMASK(); \
    __disable_irq()

#define BB_EXIT_CRITICAL() __set_PRIMASK(primask)
#endif /* STM32F1 */

#define BB_TICKS_PER_US (CPU_CLOCK_FREQ_HZ / 1000000UL)

#define BB_US_TO_TICKS(US) ((uint32_t)(US) * BB_TICKS_PER_US)

#define BB_TICKS_TO_US(TICKS) ((uint32_t)(TICKS) / (BB_TICKS_PER_US))

static inline void bb_delay_ticks(uint32_t ticks)
{
    uint32_t start, current;
    BB_GET_TICKS(start);
    do
    {
        BB_GET_TICKS(current);
    } while ((current - start) < ticks);
}
#define BB_DELAY_TICKS(TICKS) bb_delay_ticks(TICKS)

#define BB_DELAY_US(US) BB_DELAY_TICKS(BB_US_TO_TICKS((US)))
