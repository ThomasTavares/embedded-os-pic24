#include "io.h"
#include "os_config.h"
#include <xc.h>


#define FCY 4000000UL

void UART_init(uint32_t baudrate)
{
    U1MODE = 0;
    U1STA  = 0;

    U1MODEbits.BRGH = 0;

    U1BRG = (FCY / (16UL * baudrate)) - 1;

    U1MODEbits.UARTEN = 1;
    U1STAbits.UTXEN = 1;
}

void UART_write_char(char c)
{
    while (U1STAbits.UTXBF);    // wait for buffer space

    U1TXREG = c;
}

void UART_write_string(const char *str)
{
    while (*str != '\0') {
        UART_write_char(*str);
        str++;
    }
}

void UART_write_new_line(const char *str)
{
    UART_write_string(str);
    UART_write_string("\r\n");
}

void UART_write_int(int value)
{
    char buffer[8];
    int i = 0;

    if (value == 0) {
        UART_write_char('0');
        return;
    }

    if (value < 0) {
        UART_write_char('-');
        value = -value;
    }

    while (value > 0) {
        buffer[i++] = (value % 10) + '0';
        value /= 10;
    }

    while (i > 0) {
        UART_write_char(buffer[--i]);
    }
}