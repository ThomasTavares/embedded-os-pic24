/* 
 * File:   pwm.h
 * Author: thoma
 *
 * Created on 14 de Outubro de 2026, 14:31
 */

#ifndef PWM_H
#define	PWM_H

#ifndef FCY
#define FCY 4000000
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t period;
    uint16_t cycle;
    bool status;
} pwm_driver_t;

void pwm_config(pwm_driver_t *pwm, uint16_t freq);
void pwm_start(pwm_driver_t *pwm);
void pwm_stop(pwm_driver_t *pwm);
void pwm_set_duty(pwm_driver_t *pwm, uint16_t duty);

#endif	/* PWM_H */