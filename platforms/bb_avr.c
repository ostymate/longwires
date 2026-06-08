/**
 * @file bb_avr_timer.c
 * @brief Timer1 ISR and init for AVR platform
 */

#ifdef AVR

#include "bb_avr.h"

volatile uint32_t _bb_timer1_overflow = 0;
static bool _bb_timer1_init = false;

ISR(TIMER1_OVF_vect)
{
    _bb_timer1_overflow += 65536UL;
}

void BB_TICKS_SOURCE_INIT(void)
{
    if (_bb_timer1_init)
        return;
    _bb_timer1_overflow = 0;
    TCCR1A = 0;
    TCCR1B = (1 << CS10);
    TCNT1 = 0;
    TIMSK1 |= (1 << TOIE1);
    _bb_timer1_init = true;
}

#endif /* AVR */
