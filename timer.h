/* 
 * File:   timer.h
 * Author: thoma
 *
 * Created on 2 de Setembro de 2026, 14:39
 */

#ifndef TIMER_H
#define	TIMER_H

#include <xc.h>

void config_timer1();
void __attribute__ ((interrupt, no_auto_psv, naked)) _T1Interrupt(void);

#endif	/* TIMER_H */