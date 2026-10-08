#include "interrupt.h"
#include "kernel.h"
#include "scheduler.h"

extern queue_t ReadyQueue;
extern uint8_t task_running;
extern uint16_t *task_running_tos;

void config_int0()
{
    IEC0bits.INT0IE = 1;    // interruption enable
    IFS0bits.INT0IF = 0;    // interruption flag
    IPC0bits.INT0IP = 4;    // interruption priority (from 1 to 7, 0 disables)
}

void __attribute__ ((interrupt, no_auto_psv, naked)) _INT0Interrupt(void)
{
    SAVE_CONTEXT();         // save previous task context
    
    IFS0bits.INT0IF = 0;    // drop flag
    
    ReadyQueue.tasks[task_running].stack_pointer = task_running_tos;    // save stack pointer
    
    task_running = scheduler();                                         // call scheduler
    
    task_running_tos = ReadyQueue.tasks[task_running].stack_pointer;    // update stack tos
    ReadyQueue.tasks[task_running].task_state = RUNNING;                // set next task state
    
    RESTORE_CONTEXT();      // restore next task context
}