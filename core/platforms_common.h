/**
 * @file platforms_common.h
 * @brief common macros for software I2C and 1-Wire
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "../platforms/lw_esp32.h"
#include "../platforms/lw_stm32f1.h"
#include "../platforms/lw_avr.h"

/**
 * @brief wait for period of ticks.
 * @param ticks ticks to wait.
 */
static inline void delay_ticks(uint32_t ticks)
{
    uint32_t start, now;
    GET_TICK(start);
    do
    {
        GET_TICK(now);
    } while ((now - start) < (ticks));
}

#ifndef NULL
#define NULL ((void *)0)
#endif /* NULL */