#include "memory.h"
#include <stdio.h>
#include "os_config.h"
#include "types.h"

uint8_t heap_mem[HEAP_SIZE];

void os_heap_init()
{
    mem_block_info_t *blk_info = (mem_block_info_t *)heap_mem;

    blk_info->size = HEAP_SIZE - sizeof(mem_block_info_t);
    blk_info->free = 1;
}

void *os_malloc(uint16_t nr_bytes)
{
    uint8_t *heap_ptr   = heap_mem;
    uint8_t *heap_end   = heap_mem + HEAP_SIZE;

    if (nr_bytes == 0) return NULL;

    while (heap_ptr + sizeof(mem_block_info_t) <= heap_end) {   // verifies if there is available space
        mem_block_info_t *blk_info = (mem_block_info_t*)heap_mem;

        /// verifies if the block is big enough for allocation
        if (blk_info->free && blk_info->size >= nr_bytes) {
            uint16_t remaining = blk_info->size - nr_bytes;

            // verifies if the heap has space to create another block
            if (remaining > sizeof(mem_block_info_t)) {
                mem_block_info_t *new_blk_info = (mem_block_info_t *)(heap_ptr + sizeof(mem_block_info_t) + nr_bytes);
                new_blk_info->size = remaining - sizeof(mem_block_info_t);
                new_blk_info->free = 1;
                
                blk_info->size = nr_bytes;
            }

            // sets the block as occupied
            blk_info->free = 0;

            // return the address to the block data area
            return heap_ptr + sizeof(mem_block_info_t);
        }

        // goes to next block
        heap_ptr += sizeof(mem_block_info_t) + blk_info->size;
    }

    // no available space found
    return NULL;
}

void os_free(void *address)
{
    if (address == NULL) return;

    mem_block_info_t *blk_info = (mem_block_info_t *)((char *)address - sizeof(mem_block_info_t));

    blk_info->free = 1;
}