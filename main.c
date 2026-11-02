/*
 * Basic OS Project
 * Created on 2 de Setembro de 2026, 14:36
 */

#include "kernel.h"
#include "user_app.h"

int main(void)
{
    // init SO data structures
    os_config();
    
    // create user tasks
    
    /* create_task(task_A, 5);
    //create_task(task_B, 5);
    create_task(task_C, 5); */
    
    create_task(task_pwm,       4); // high prior
    create_task(task_producer,  3); // medium prior
    create_task(task_consumer,  3); // medium prior
    create_task(task_adc,       2); // low prior
    
    // init scheduler
    os_start_scheduler();
    
    while (1);
    return 0;
}