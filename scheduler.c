#include <builtins.h>

#include "scheduler.h"
#include "os_config.h"
#include "types.h"
#include "kernel.h"

extern queue_t ReadyQueue;
extern uint8_t task_running;
extern uint16_t *task_running_tos;

uint8_t scheduler()
{
#if DEFAULT_SCHEDULER == RR_SCHEDULER
    return rr_scheduler();
#else
    return prior_scheduler();
#endif
}

uint8_t rr_scheduler()
{
    uint8_t next_task = task_running, idle_count = 0;
    do {
        //next_task = (next_task + 1) % ReadyQueue.queue_size;
        next_task = __builtin_modsd(next_task+1, ReadyQueue.queue_size);
        if (ReadyQueue.tasks[next_task].task_func == idle) {
            idle_count++;
            if (idle_count >= 2) return 0;
        }
    } while (ReadyQueue.tasks[next_task].task_state != READY ||
             ReadyQueue.tasks[next_task].task_func == idle);
    
    return next_task;
}

uint8_t prior_scheduler()
{
    
}