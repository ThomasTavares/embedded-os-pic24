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

TASK task_button();
TASK task_cmd();
TASK task_adc();
TASK task_pwm();
TASK task_rr_a();
TASK task_rr_b();

#endif	/* USER_APP_H */

