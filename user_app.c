#include "user_app.h"
#include <xc.h>
#include "interrupt.h"
#include "kernel.h"
#include "sync.h"
#include "message.h"
#include "uart.h"
#include "pwm.h"

m_queue_t message_queue;

void config_user_app()
{
    TRISEbits.TRISE0 = 0;   // 0 = pin as output
    TRISEbits.TRISE1 = 0;
    TRISEbits.TRISE2 = 0;
    TRISFbits.TRISF3 = 0;
    m_queue_init(&message_queue);
}

TASK task_A()
{
    char chars[4] = {'a', 'b', 'c', 'd'};
    int index = 0;
    
    while (1) {
        LATEbits.LATE0 ^= 1;
        m_queue_write(&message_queue, chars[index++]);
        if (index == 4) index = 0;
    }
}

TASK task_B()
{
    /* pwm_driver_t pwm;
    
    pwm_config(&pwm, 1000);
    pwm_start(&pwm);
    pwm_set_duty(&pwm, 75); // 75% */
    
    while (1) {
        LATEbits.LATE1 ^= 1;
    }
    
    // pwm_stop(&pwm);
}

TASK task_C()
{
    char output;
    while (1) {
        LATEbits.LATE2 ^= 1;
        m_queue_read(&message_queue, &output);
    }
}