/*
 * File:   adc.c
 * Author: gabri
 *
 * Created on November 2, 2026, 9:08 AM
 */

#ifndef ADC_H
#define	ADC_H

#include <stdbool.h>
#include <stdint.h>

#define ADC_MAX_VALUE 1023u

void adc_init(void);
bool adc_select_channel(uint8_t channel);
bool adc_read(uint16_t *value);

#endif /* ADC_H */