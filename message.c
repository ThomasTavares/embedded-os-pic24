#include <xc.h>
#include <stddef.h>

#include "message.h"
#include "sync.h"
#include "memory.h"

bool m_queue_init(m_queue_t *queue)
{
    if(queue == NULL || queue->queue != NULL){
        return false;
    }
   
    queue->queue = os_malloc(
        SIZE_MESSAGE_QUEUE * sizeof(*queue->queue)
    );
    
    if(queue->queue == NULL){
        return false;
    }
    
    queue->pos_read     = 0;
    queue->pos_write    = 0;
    sem_init(&queue->s_write, SIZE_MESSAGE_QUEUE);  // init write semaphore with 4 (4 instances)
    sem_init(&queue->s_read, 0);                    // init read semaphore with 0 (0 instances)

    return true;
}

bool m_queue_write(m_queue_t *queue, char msg)
{
    uint16_t saved_ipl;
    
    if(queue == NULL || queue->queue == NULL){
        return false;
    }
    
    sem_wait(&queue->s_write);                      // pode bloquear: IPL tem que estar normal aqui
    
    SET_AND_SAVE_CPU_IPL(saved_ipl, 7);             // protege so o buffer e os indices
    queue->queue[queue->pos_write] = msg;
    queue->pos_write = __builtin_modsd(queue->pos_write+1, SIZE_MESSAGE_QUEUE);
    RESTORE_CPU_IPL(saved_ipl);
    
    sem_post(&queue->s_read);                       // avisa que ha mensagem nova

    return true;
}

bool m_queue_read(m_queue_t *queue, char *msg)
{
    uint16_t saved_ipl;
    
    if(queue == NULL || queue->queue == NULL || msg == NULL){
        return false;
    }
    
    sem_wait(&queue->s_read);                       // espera haver mensagem
    
    SET_AND_SAVE_CPU_IPL(saved_ipl, 7);
    *msg = queue->queue[queue->pos_read];
    queue->pos_read = __builtin_modsd(queue->pos_read+1, SIZE_MESSAGE_QUEUE);
    RESTORE_CPU_IPL(saved_ipl);
    
    sem_post(&queue->s_write);                      // libera uma vaga

    return true;
}

void m_queue_destroy(m_queue_t *queue){
    
    if(queue == NULL){
        return;
    }
    os_free(queue->queue);
    
    queue->queue = NULL;
    queue->pos_read = 0;
    queue->pos_write = 0;
}