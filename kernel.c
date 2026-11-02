#include "kernel.h"
#include <stdio.h>
#include <xc.h>
#include "user_app.h"
#include "timer.h"
#include "interrupt.h"
#include "memory.h"

queue_t ReadyQueue;
uint8_t task_running = 0;
uint16_t *task_running_tos = NULL;  // pointer to the top of the task stack (task stack pointer)

void os_config()
{
    ReadyQueue.queue_size = 0;

    # if DYNAMIC_ALLOC
    os_heap_init();
    #endif

    config_timer1();
    config_int0();
    config_user_app();
    
    create_task(idle, 4);
}

void os_start_scheduler()
{
    task_running_tos = ReadyQueue.tasks[0].stack_pointer;
    T1CONbits.TON = 1;
    RESTORE_CONTEXT();
}

void create_task(callback task, uint8_t priority)
{
    tcb_t *new_task = &ReadyQueue.tasks[ReadyQueue.queue_size];
    
    new_task->task_id       = ReadyQueue.queue_size+1;
    new_task->task_func     = task;
    new_task->task_priority = priority;
    new_task->original_priority = priority;
    new_task->task_state    = READY;
    new_task->task_delay    = 0;
    
    new_task->stack_pointer = &new_task->task_stack[0];
    init_stack(new_task);
    
    ReadyQueue.queue_size++;
}

void init_stack(tcb_t *task)
{
    uint16_t pc_low, pc_high, *sp;
    
    sp = task->stack_pointer;
    
    pc_low  = (uint16_t) task->task_func;
    pc_high = 0x0000;
    
    // init task func pointer
    *sp++ = pc_low;
    *sp++ = pc_high;
    
    for (int i = 0; i < 15; i++) {  // init W registers (W0 to W14)
        *sp++ = 0x0000;
    }
    
    *sp++ = 0x0000;     // init RCOUNT
    *sp++ = TBLPAG;     // init TBLPAG
    *sp++ = PSVPAG;     // init PSVPAG
    *sp++ = CORCON;     // init CORCON
    
    task->stack_pointer = sp;       // save stack pointer
}

void task_yield()
{
    __builtin_disable_interrupts();
    ReadyQueue.tasks[task_running].task_state = READY;  // set task state
    CONTEXT_SWITCH();                                   // force context switch
    __builtin_enable_interrupts();
}

void task_delay(uint16_t time)
{
    __builtin_disable_interrupts();
    ReadyQueue.tasks[task_running].task_delay = time;   // set delay
    ReadyQueue.tasks[task_running].task_state = WAITING;// set task state
    CONTEXT_SWITCH();                                   // force context switch
    __builtin_enable_interrupts();
}

void task_release()
{
    for (int i = 0; i < ReadyQueue.queue_size; i++) {
        if (ReadyQueue.tasks[i].task_state == WAITING) {
            if (ReadyQueue.tasks[i].task_delay > 0)
                ReadyQueue.tasks[i].task_delay--;
            if (ReadyQueue.tasks[i].task_delay == 0)
                ReadyQueue.tasks[i].task_state = READY;
        }
    }
}

TASK idle()
{
    while (1) {
        asm("NOP");
        Nop();
    }
}