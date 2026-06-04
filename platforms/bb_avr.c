/**
 * @file bb_avr.c
 * @brief Timer1 ISR and init for AVR platform
 */

#ifdef AVR

#include "bb_avr.h"
#include <avr/interrupt.h>

volatile uint16_t _bb_timer1_overflow;

ISR(TIMER1_OVF_vect)
{
    _bb_timer1_overflow++;
}

void bb_avr_timer_init(void)
{
    _bb_timer1_overflow = 0;
    TCCR1A = 0;
    TCCR1B = (1 << CS10);
    TCNT1 = 0;
    TIMSK1 |= (1 << TOIE1);
}

#endif /* AVR */