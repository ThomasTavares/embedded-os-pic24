#include <xc.h>
#include <libpic30.h>

#include "pwm.h"

void pwm_config(pwm_driver_t *pwm, uint16_t freq)
{
    if (freq == 0) return;
    
    pwm->period = (FCY / freq) - 1;
    pwm->cycle  = 0;
    pwm->status = false;
    
    LATDbits.LATD0 = 0;
    TRISDbits.TRISD0 = 0;
    
    OC1CON = 0;
    T2CON = 0;
    OC1CONbits.OCTSEL = 0;  // timer 2
    OC1CONbits.OCM = 0b000; // capture compare disable
    T2CONbits.TCKPS = 0;    // preescaler 1:1

    OC1R   = 0;
    OC1RS  = 0;
    TMR2   = 0;
}

void pwm_start(pwm_driver_t *pwm)
{
    if (pwm->status) return;
    OC1CONbits.OCM = 0b000;
    T2CONbits.TON  = 0;

    TMR2 = 0;
    PR2  = pwm->period;

    pwm->status = true;
    
    OC1R  = pwm->cycle;
    OC1RS = pwm->cycle;
    
    OC1CONbits.OCM = 0b110;
    T2CONbits.TON  = 1;
}

void pwm_stop(pwm_driver_t *pwm)
{
    if (!pwm->status) return;
    pwm->status = false;
    
    OC1CONbits.OCM = 0b000;
    T2CONbits.TON  = 0;
}

void pwm_set_duty(pwm_driver_t *pwm, uint16_t duty)
{
    if (!pwm->status) return;

    if (duty > 100u) duty = 100u;
    uint32_t cycle = ((uint32_t)(pwm->period + 1UL) * duty)/100UL;
    //if (cycle > pwm->period) cycle = pwm->period;
    
    pwm->cycle = (uint16_t)cycle;
    //OC1R  = pwm->cycle;
    OC1RS = pwm->cycle;
    //TMR2  = 0;
}