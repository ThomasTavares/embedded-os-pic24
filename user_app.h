/* 
 * File:   user_app.h
 * Author: thoma
 *
 * Created on 2 de Setembro de 2026, 14:39
 */

#ifndef USER_APP_H
#define	USER_APP_H

#include "types.h"

void config_user_app();
void validate_message_queue();

/* TASK task_A();
TASK task_B();
TASK task_C();
TASK task_adc_test(void); */

TASK task_producer();
TASK task_consumer();
TASK task_adc();
TASK task_pwm();

#endif	/* USER_APP_H */