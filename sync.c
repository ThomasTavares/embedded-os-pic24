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
    m->owner_id         = 255;
    m->is_locked        = 0;
    m->m_queue_input    = 0;
    m->m_queue_output   = 0;
}

void mutex_lock(mutex_t *m)
{
    __builtin_disable_interrupts();
    
    if (m->is_locked == 0) {
        m->is_locked = 1;
        m->owner_id = task_running;
    } else {
        // priority inheritance logic
        if (ReadyQueue.tasks[task_running].task_priority > ReadyQueue.tasks[m->owner_id].task_priority) {
            ReadyQueue.tasks[m->owner_id].task_priority = ReadyQueue.tasks[task_running].task_priority;
        }
        
        m->mutex_queue[m->m_queue_input] = task_running;
        m->m_queue_input = __builtin_modsd(m->m_queue_input + 1, MAX_TASKS);
        ReadyQueue.tasks[task_running].task_state = BLOCKED;
        CONTEXT_SWITCH();
    }
    
    __builtin_enable_interrupts();
}

void mutex_unlock(mutex_t *m)
{
    __builtin_disable_interrupts();
    
    if (m->owner_id == task_running) {
        // restore original priority
        ReadyQueue.tasks[task_running].task_priority = ReadyQueue.tasks[task_running].original_priority;
        
        if (m->m_queue_input == m->m_queue_output) {
            // queue is empty, free mutex
            m->is_locked = 0;
            m->owner_id = 255;
        } else {
            // pass ownership to the next blocked task
            uint8_t next_task = m->mutex_queue[m->m_queue_output];
            m->m_queue_output = __builtin_modsd(m->m_queue_output + 1, MAX_TASKS);
            
            m->owner_id = next_task;
            ReadyQueue.tasks[next_task].task_state = READY;
            
            // yield cpu
            ReadyQueue.tasks[task_running].task_state = READY;
            CONTEXT_SWITCH();
        }
    }
    
    __builtin_enable_interrupts();
}