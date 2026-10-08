#include "sync.h"
#include "interrupt.h"
#include <xc.h>

extern queue_t ReadyQueue;
extern uint8_t task_running;
extern uint16_t *task_running_tos;

void sem_init(sem_t *s, uint8_t value)
{
    s->sem_count        = value;
    s->s_queue_input    = 0;
    s->s_queue_output   = 0;
}

void sem_wait(sem_t *s)
{
    __builtin_disable_interrupts();
    
    s->sem_count--;
    
    if (s->sem_count < 0) { // block task
        s->sem_queue[s->s_queue_input] = task_running;
        s->s_queue_input = __builtin_modsd(s->s_queue_input+1, MAX_TASKS);
        ReadyQueue.tasks[task_running].task_state = BLOCKED;
        CONTEXT_SWITCH();
    }
    
    __builtin_enable_interrupts();
}

void sem_post(sem_t *s)
{
    __builtin_disable_interrupts();
    
    s->sem_count++;
    
    if (s->sem_count <= 0) {
        ReadyQueue.tasks[s->sem_queue[s->s_queue_output]].task_state = READY;
        s->s_queue_output = __builtin_modsd(s->s_queue_output+1, MAX_TASKS);
        ReadyQueue.tasks[task_running].task_state = READY;
        CONTEXT_SWITCH();
    }
    
    __builtin_enable_interrupts();  
}

void mutex_init(mutex_t *m)
{
    
}

void mutex_lock(mutex_t *m)
{
    
}

void mutex_unlock(mutex_t *m)
{
    
}