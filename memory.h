/* 
 * File:   memory.h
 * Author: thoma
 *
 * Created on 28 de Setembro de 2026, 15:19
 */

#ifndef MEMORY_H
#define	MEMORY_H

#include <stdint.h>

void os_heap_init();
void *os_malloc(uint16_t nr_bytes);
void os_free(void *address);

#endif	/* MEMORY_H */