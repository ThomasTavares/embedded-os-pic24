/* 
 * File:   scheduler.h
 * Author: thoma
 *
 * Created on 2 de Setembro de 2026, 14:38
 */

#ifndef SCHEDULER_H
#define	SCHEDULER_H

#include <stdint.h>

uint8_t scheduler();
uint8_t rr_scheduler();
uint8_t prior_scheduler();

void    scheduler_set(uint8_t type);   // RR_SCHEDULER ou PRIOR_SCHEDULER
uint8_t scheduler_get(void);

#endif	/* SCHEDULER_H */

