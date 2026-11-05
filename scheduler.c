#include <builtins.h>

#include "scheduler.h"
#include "os_config.h"
#include "types.h"
#include "kernel.h"

extern queue_t ReadyQueue;
extern uint8_t task_running;
extern uint16_t *task_running_tos;

// escalonador ativo: comeca com DEFAULT_SCHEDULER e pode ser trocado em execucao
static volatile uint8_t active_scheduler = DEFAULT_SCHEDULER;

void scheduler_set(uint8_t type)
{
    if (type == RR_SCHEDULER || type == PRIOR_SCHEDULER) {
        active_scheduler = type;    // escrita de 1 byte: atomica
    }
}

uint8_t scheduler_get(void)
{
    return active_scheduler;
}

uint8_t scheduler()
{
    if (active_scheduler == PRIOR_SCHEDULER) {
        return prior_scheduler();
    }
    return rr_scheduler();
}

uint8_t rr_scheduler()
{
    uint8_t next_task = task_running, idle_count = 0;
    do {
        next_task = __builtin_modsd(next_task + 1, ReadyQueue.queue_size);
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
    uint8_t i, next_task;
    uint8_t selected_task = 0;      // fallback to idle task (index 0)
    int16_t highest_priority = -1;  // -1 guarantees any valid priority (>=0) is selected
    
    // iterate starting from the next task to ensure Round-Robin
    for (i = 1; i <= ReadyQueue.queue_size; i++) {
        next_task = __builtin_modsd(task_running + i, ReadyQueue.queue_size);
        
        // skip idle task on normal search
        if (ReadyQueue.tasks[next_task].task_func == idle) {
            continue;
        }
        
        if (ReadyQueue.tasks[next_task].task_state == READY || ReadyQueue.tasks[next_task].task_state == RUNNING) {
            // ensures the first found task is kept
            if ((int16_t)ReadyQueue.tasks[next_task].task_priority > highest_priority) {
                highest_priority = (int16_t)ReadyQueue.tasks[next_task].task_priority;
                selected_task = next_task;
            }
        }
    }
    
    return selected_task;
}