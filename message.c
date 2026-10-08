#include "message.h"
#include "sync.h"

void m_queue_init(m_queue_t *queue)
{
    queue->pos_read     = 0;
    queue->pos_write    = 0;
    sem_init(&queue->s_write, SIZE_MESSAGE_QUEUE);  // init write semaphore with 4 (4 instances)
    sem_init(&queue->s_read, 0);                    // init read semaphore with 0 (0 instances)
                                                    // (blocks if first initialized)
}

void m_queue_write(m_queue_t *queue, char msg)
{
    sem_wait(&queue->s_write);                                      // wait for write semaphore
    queue->queue[queue->pos_write] = msg;                           // get the message to be wriiten
    queue->pos_write = __builtin_modsd(queue->pos_write+1, SIZE_MESSAGE_QUEUE); // adjust position
    sem_post(&queue->s_read);                                       // post for the read semaphore, unblocking it
}

void m_queue_read(m_queue_t *queue, char *msg)
{
    sem_wait(&queue->s_read);                                       // wait for read semaphore
    *msg = queue->queue[queue->pos_read];                           // read the message
    queue->pos_read = __builtin_modsd(queue->pos_read+1, SIZE_MESSAGE_QUEUE);   // adjust position 
    sem_post(&queue->s_write);                                      // post for the write semaphore, unblocking it
}