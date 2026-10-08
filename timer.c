#include "timer.h"
#include "kernel.h"
#include "scheduler.h"

extern queue_t ReadyQueue;
extern uint8_t task_running;
extern uint16_t *task_running_tos;

void config_timer1()
{
    T1CONbits.TCS = 0;      // internal clock
    T1CONbits.TCKPS = 0b01; // preescale 1:8
    IFS0bits.T1IF = 0;      // interruption flag
    IEC0bits.T1IE = 1;      // interruption enable
    IPC0bits.T1IP = 4;      // interruption priority (from 1 to 7, 0 disables)
    PR1 = 1999;             // period to interrupt (= 1 ms)
}

void __attribute__ ((interrupt, no_auto_psv, naked)) _T1Interrupt(void)
{
    SAVE_CONTEXT();         // save previous task context
    
    IFS0bits.T1IF = 0;      // drop flag
    
    ReadyQueue.tasks[task_running].stack_pointer = task_running_tos;    // save stack pointer
    ReadyQueue.tasks[task_running].task_state = READY;                  // set previous task state
    
    task_release();                                                     // release task from delay (if time has met)  
    task_running = scheduler();                                         // call scheduler
    
    task_running_tos = ReadyQueue.tasks[task_running].stack_pointer;    // update stack tos
    ReadyQueue.tasks[task_running].task_state = RUNNING;                // set next task state
    
    RESTORE_CONTEXT();      // restore next task context
}