/**
 * @file bb_utils.h
 * @brief common macros for software I2C and 1-Wire
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "../platforms/bb_esp32.h"
#include "../platforms/bb_stm32f1.h"
#include "../platforms/bb_avr.h"

/**
 * @brief Busy-wait for the given number of ticks.
 * @param TICKS Number of ticks to wait.
 */
static inline void BB_DELAY_TICKS(uint32_t TICKS)
{
    uint32_t _bb_delay_ticks_start, _bb_delay_ticks_now;
    BB_GET_TICKS(_bb_delay_ticks_start);
    do
    {
        BB_GET_TICKS(_bb_delay_ticks_now);
    } while ((_bb_delay_ticks_now - _bb_delay_ticks_start) < (TICKS));
}

#ifndef NULL
#define NULL ((void *)0)
#endif /* NULL */