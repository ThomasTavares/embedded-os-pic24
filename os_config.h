/* 
 * File:   os_config.h
 * Author: thoma
 *
 * Created on 2 de Setembro de 2026, 14:45
 */

#ifndef OS_CONFIG_H
#define	OS_CONFIG_H

#define FCY         4000000UL

#define ON  1
#define OFF 0

#define MAX_TASKS           7   // max amount of tasks
#define MAX_STAK_SIZE     128   // the size of the tcb stack (might need to increase with project complexity)

#define SIZE_MESSAGE_QUEUE  4   // max amount of messages on queue

#define DYNAMIC_ALLOC      ON
#define HEAP_SIZE        1024   // size of the heap region of memory

#define RR_SCHEDULER        1   // round-robin task scheduler
#define PRIOR_SCHEDULER     2   // priority based task scheduler

#define DEFAULT_SCHEDULER   PRIOR_SCHEDULER   // escalonador inicial (pode ser trocado em execucao)

#endif	/* OS_CONFIG_H */