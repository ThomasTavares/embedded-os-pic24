#ifndef SYNC_H
#define	SYNC_H

#include "types.h"

void sem_init(sem_t *s, uint8_t value);
void sem_wait(sem_t *s);
void sem_post(sem_t *s);

void mutex_init(mutex_t *m);
void mutex_lock(mutex_t *m);
void mutex_unlock(mutex_t *m);

#endif	/* SYNC_H */