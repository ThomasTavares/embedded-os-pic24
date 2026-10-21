#include "uart.h"
#include "os_config.h"
#include <xc.h>

#define FCY 4000000UL

void UART_init(uint32_t baudrate)
{
    if (baudrate == 0) return;
    
    // Configura divisor do oscilador FRC para 1:1
    CLKDIVbits.RCDIV = 0;

    // Desabilita UART1 durante a configuração.
    U1MODEbits.UARTEN = 0;

    // Reinicializa os registradores.
    U1MODE  = 0;
    U1STA   = 0;

    // Configuração: 8 bits, sem paridade, 1 stop bit.
    U1MODEbits.PDSEL = 0;
    U1MODEbits.STSEL = 0;

    // Modo de baixa velocidade (divisor 16).
    U1MODEbits.BRGH = 0;

    // Configura baud rate.
    U1BRG = (FCY / (16UL * baudrate)) - 1;

    // Habilita a UART.
    U1MODEbits.UARTEN = 1;

    // Habilita o transmissor.
    U1STAbits.UTXEN = 1;
}

void UART_write_char(char c)
{
    // Aguarda espaço no buffer de transmissão
    while (U1STAbits.UTXBF);

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