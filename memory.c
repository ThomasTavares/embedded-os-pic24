#include "memory.h"
#include <stdio.h>
#include <xc.h>
#include "os_config.h"
#include "types.h"

static uint8_t heap_mem[HEAP_SIZE] __attribute__((aligned(2)));

void os_heap_init(void)
{
    uint16_t saved_ipl;
    
    SET_AND_SAVE_CPU_IPL(saved_ipl, 7);
 
    mem_block_info_t *blk_info = (mem_block_info_t *)heap_mem;
 
    blk_info->size = HEAP_SIZE - sizeof(mem_block_info_t);
    blk_info->free = 1;
 
    RESTORE_CPU_IPL(saved_ipl);
}

void *os_malloc(uint16_t nr_bytes)
{
    uint8_t *heap_ptr   = heap_mem;
    uint8_t *heap_end   = heap_mem + HEAP_SIZE;
    void    *result   = NULL;
    uint16_t saved_ipl;

    if (nr_bytes == 0 || nr_bytes > HEAP_SIZE) return NULL;
    
    // round up to an even size so the next block header stays word-aligned
    nr_bytes = (uint16_t)((nr_bytes + 1u) & ~1u);
    
    SET_AND_SAVE_CPU_IPL(saved_ipl, 7);

    while (heap_ptr + sizeof(mem_block_info_t) <= heap_end) {   // verifies if there is available space
        mem_block_info_t *blk_info = (mem_block_info_t*)heap_ptr;

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

            // sets the address to the block data area
            result = heap_ptr + sizeof(mem_block_info_t);
            break;
        }

        // goes to next block
        heap_ptr += sizeof(mem_block_info_t) + blk_info->size;
    }
    
    RESTORE_CPU_IPL(saved_ipl);

    // NULL if no available space was found
    return result;
}

void os_free(void *address)
{
    uint8_t *ptr      = (uint8_t *)address;
    uint8_t *heap_end = heap_mem + HEAP_SIZE;
    uint16_t saved_ipl;
    
    if (ptr == NULL) return;
    
    // reject pointers outside the heap or misaligned (cannot come from os_malloc)
    if (ptr < heap_mem + sizeof(mem_block_info_t) ||
        ptr >= heap_end ||
        ((uintptr_t)ptr & 1u)) return;
    
    SET_AND_SAVE_CPU_IPL(saved_ipl, 7);

    // mark the block as free
    mem_block_info_t *blk_info = (mem_block_info_t *)(ptr - sizeof(mem_block_info_t));
    
    // double free protection
    if (blk_info->free) {
        RESTORE_CPU_IPL(saved_ipl);
        return;
    }
    
    // mark the block as free
    blk_info->free = 1;

    // merge adjacent free blocks
    uint8_t *heap_ptr = heap_mem;
 
    while (heap_ptr + sizeof(mem_block_info_t) <= heap_end) {
        mem_block_info_t *current_blk = (mem_block_info_t *)heap_ptr;
 
        if (current_blk->free) {
            uint8_t *next_ptr = heap_ptr + sizeof(mem_block_info_t) + current_blk->size;
 
            // if there is a next block and it is within heap bounds
            if (next_ptr + sizeof(mem_block_info_t) <= heap_end) {
                mem_block_info_t *next_blk = (mem_block_info_t *)next_ptr;
 
                if (next_blk->free) {
                    // add next block's size and its header size to the current block
                    current_blk->size += sizeof(mem_block_info_t) + next_blk->size;
 
                    // check if the new merged block can be merged with the next one
                    continue;
                }
            }
        }
 
        // go to next block
        heap_ptr += sizeof(mem_block_info_t) + current_blk->size;
    }
 
    RESTORE_CPU_IPL(saved_ipl);
}