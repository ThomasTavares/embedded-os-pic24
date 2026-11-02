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

TASK task_A();
TASK task_B();
TASK task_C();
TASK task_adc_test(void);

#endif	/* USER_APP_H */

