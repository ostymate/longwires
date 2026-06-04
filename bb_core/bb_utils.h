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
#endif /* AVR */

#ifndef NULL
#define NULL ((void *)0)
#endif /* NULL */
