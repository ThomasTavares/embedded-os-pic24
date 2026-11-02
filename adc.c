/*
 * File:   adc.c
 * Author: gabri
 *
 * Created on November 2, 2026, 9:08 AM
 */


#include "adc.h"
#include "os_config.h"

#include <stddef.h>
#include <libpic30.h>
#include <xc.h>

void adc_init(void)
{
    AD1CON1 = 0;
    AD1CON2 = 0;
    AD1CON3 = 0;
    
    AD1CHS = 0;
    AD1CSSL = 0;

    AD1PCFG = 0xFFFF;

    IEC0bits.AD1IE = 0;
    IFS0bits.AD1IF = 0;

    AD1CON1bits.SSRC = 7;
    AD1CON2bits.VCFG = 0;
    AD1CON3bits.ADRC = 0;

    AD1CON3bits.ADCS = 7;
    AD1CON3bits.SAMC = 16;

    (void)adc_select_channel(0);
}

bool adc_select_channel(uint8_t channel)
{
    uint16_t mask;

    if (channel > 15u) {
        return false;
    }
    mask = (uint16_t)(1u << channel);

    AD1CON1bits.ADON = 0;
    TRISB |= mask;
    AD1PCFG &= (uint16_t)~mask;
    AD1CHSbits.CH0SA = channel;
    AD1CHSbits.CH0NA = 0;
    AD1CON1bits.ADON = 1;

    __delay_us(20);

    return true;
}

bool adc_read(uint16_t *value)
{
    uint16_t timeout = 60000u;
    // uint32_t timeout = 600000ul;

    if (value == NULL || !AD1CON1bits.ADON) {
        return false;
    }

    // AD1CON1bits.DONE = 0;
    AD1CON1bits.SAMP = 1;

    while (!AD1CON1bits.DONE) {
        if (--timeout == 0u) {
            AD1CON1bits.SAMP = 0;
            return false;
        }
    }

    *value = ADC1BUF0;

    return true;
}