/* 
 * File:   types.h
 * Author: thoma
 *
 * Created on 2 de Setembro de 2026, 14:38
 */

#ifndef TYPES_H
#define	TYPES_H

#include "os_config.h"
#include <stdint.h>

typedef void (*callback) (void);        // pointer for the task function

typedef void TASK;                      // label type for tasks

typedef enum {                          // available task states
    READY = 0,
    RUNNING,
    WAITING,
    BLOCKED
} state_t;

typedef struct {                        // struct for the tcb
    uint8_t     task_id;
    uint8_t     task_priority;
    uint16_t    task_delay;
    callback    task_func;
    state_t     task_state;
    uint16_t    task_stack[MAX_STAK_SIZE];  // tcb stack of 16 bits values
    uint16_t    *stack_pointer;             // pointer to the top of stack
} tcb_t;

typedef struct {                        // struct for the tasks queue
    tcb_t   tasks[MAX_TASKS+1];
    uint8_t queue_size;                 // number of tasks on the array
} queue_t;

typedef struct {                        // struct for the semaphore
    uint8_t sem_queue[MAX_TASKS];
    int     sem_count;                  // counter (if max == 1 => mutex)
    uint8_t s_queue_input;              // input index
    uint8_t s_queue_output;             // output index
} sem_t;

typedef struct {
    // owner task (id)
} mutex_t;

typedef struct {                        // struct for the queue of messages
    char    queue[SIZE_MESSAGE_QUEUE];
    uint8_t pos_write;
    uint8_t pos_read;
    sem_t   s_write;                    // write control semaphore
    sem_t   s_read;                     // read control semaphore
} m_queue_t;

typedef struct {                        // struct for the heap memory block info
    uint16_t size;                      // size of the block in bytes
    uint8_t free;                       // boolean (1 = free, 0 = occupied)
} mem_block_info_t;

#endif	/* TYPES_H */