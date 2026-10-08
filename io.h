/* 
 * File:   io.h
 * Author: thoma
 *
 * Created on 30 de Setembro de 2026, 13:17
 */

#ifndef IO_H
#define	IO_H

#include <stdint.h>

// UART
void UART_init(uint32_t baudrate);
void UART_write_char(char c);

void UART_write_string(const char *str);
void UART_write_new_line(const char *str);
void UART_write_int(int value);

#endif	/* IO_H */