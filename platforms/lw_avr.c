/**
 * @file lw_avr.c
 * @brief Timer1 ISR and init for AVR platform
 */

#ifdef AVR

#include "lw_avr.h"

volatile uint32_t _timer1_overflow = 0;
static bool _timer1_init = false;

ISR(TIMER1_OVF_vect)
{
    _timer1_overflow += 65536UL;
}

void TICK_INIT(void)
{
    if (_timer1_init)
        return;
    _timer1_overflow = 0;
    TCCR1A = 0;
    TCCR1B = (1 << CS10);
    TCNT1 = 0;
    TIMSK1 |= (1 << TOIE1);
    _timer1_init = true;
}

#endif /* AVR */
