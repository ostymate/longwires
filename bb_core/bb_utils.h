/**
 * @file bb_utils.h
 * @brief common macros for software I2C and 1-Wire
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef ESP_PLATFORM
#include "../platforms/bb_esp32.h"
#endif /* ESP_PLATFORM */

#ifdef STM32F1
#include "../platforms/bb_stm32f1.h"
#endif /* STM32F1 */

#ifdef AVR
#include "../platforms/bb_avr.h"
#endif

#ifndef NULL 
#define NULL ((void *)0)
#endif

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

