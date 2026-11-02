/* 
 * File:   message.h
 * Author: thoma
 *
 * Created on 28 de Setembro de 2026, 13:11
 */

#ifndef MESSAGE_H
#define	MESSAGE_H

#include <stdbool.h>
#include "types.h"

bool m_queue_init(m_queue_t *queue);
bool m_queue_write(m_queue_t *queue, char msg);
bool m_queue_read(m_queue_t *queue, char *msg);
void m_queue_destroy(m_queue_t *queue);

#endif	/* MESSAGE_H */