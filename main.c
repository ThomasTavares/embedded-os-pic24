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
    create_task(task_pwm,       4); // high prior
    create_task(task_button,    3); // medium prior (produtora da fila)
    create_task(task_cmd,       3); // medium prior (consumidora da fila)
    create_task(task_adc,       2); // low prior
    create_task(task_rr_a,      1); // lowest prior (round-robin)
    create_task(task_rr_b,      1); // lowest prior (round-robin)
    
    // init scheduler
    os_start_scheduler();
    
    while (1);
    return 0;
}