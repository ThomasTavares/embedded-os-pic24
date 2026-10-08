/* 
 * File:   interrupt.h
 * Author: thoma
 *
 * Created on 14 de Setembro de 2026, 14:54
 */

#ifndef INTERRUPT_H
#define	INTERRUPT_H

#include <xc.h>

#define CONTEXT_SWITCH() IFS0bits.INT0IF = 1    // force context switch from int0 interruption

void config_int0();
void __attribute__ ((interrupt, no_auto_psv, naked)) _INT0Interrupt(void);

#endif	/* INTERRUPT_H */

