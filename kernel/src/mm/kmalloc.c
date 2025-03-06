/*
 * FreeCore - A free operating system kernel
 * Copyright (C) 2025 FreeCore Development Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <lib/minstd.h>
#include <kernel/io.h>
#include <mm/kmalloc.h>

/* Size of the kernel heap (4 MB) */
#define KERNEL_HEAP_SIZE (4 * 1024 * 1024)

static uint8_t kernel_heap[KERNEL_HEAP_SIZE] __attribute__((aligned(16)));

/* Allocation header */
typedef struct alloc_header {
    size_t size;       /* Size of allocation including header */
    uint32_t magic;    /* Magic number for validation */
    bool used;         /* Whether this block is in use */
} alloc_header_t;

/* Magic number for validation */
#define ALLOC_MAGIC 0xABCD1234

/* Minimum allocation size */
#define MIN_ALLOC_SIZE (sizeof(alloc_header_t) + 16)

/* Tracking variables */
static bool heap_initialized = false;
static size_t heap_used = 0;

/**
 * Initialize the kernel heap
 * @return 0 on success, negative on error
 */
int kmalloc_init(void) {
    if (heap_initialized) {
        return 0;
    }

    kprintf("MM: Initializing kernel heap\n");

    /* Clear the heap */
    memset(kernel_heap, 0, KERNEL_HEAP_SIZE);

    /* Initialize the first free block */
    alloc_header_t *first_block = (alloc_header_t *)kernel_heap;
    first_block->size = KERNEL_HEAP_SIZE;
    first_block->magic = ALLOC_MAGIC;
    first_block->used = false;

    heap_used = 0;
    heap_initialized = true;

    kprintf("MM: Kernel heap initialized, size: %d bytes\n", KERNEL_HEAP_SIZE);
    return 0;
}

/**
 * Find a free block of sufficient size
 * @param size Size needed (including header)
 * @return Pointer to free block, or NULL if none found
 */
static alloc_header_t *find_free_block(size_t size) {
    alloc_header_t *current = (alloc_header_t *)kernel_heap;
    alloc_header_t *heap_end = (alloc_header_t *)(kernel_heap + KERNEL_HEAP_SIZE);

    while ((uintptr_t)current < (uintptr_t)heap_end) {
        /* Validate the block */
        if (current->magic != ALLOC_MAGIC) {
            /* Corrupted heap */
            kerr("MM: Corrupted heap detected at 0x%p\n", current);
            return NULL;
        }

        /* Check if this block is free and big enough */
        if (!current->used && current->size >= size) {
            return current;
        }

        /* Move to the next block */
        current = (alloc_header_t *)((uintptr_t)current + current->size);
    }

    return NULL;
}

/**
 * Split a block if needed
 * @param block Block to split
 * @param size Size needed (including header)
 */
static void split_block(alloc_header_t *block, size_t size) {
    if (block->size >= size + MIN_ALLOC_SIZE) {
        /* Enough space to split */
        alloc_header_t *new_block = (alloc_header_t *)((uintptr_t)block + size);
        new_block->size = block->size - size;
        new_block->magic = ALLOC_MAGIC;
        new_block->used = false;

        /* Update the original block */
        block->size = size;
    }

    /* Mark the block as used */
    block->used = true;
}

/**
 * Allocate memory from the kernel heap
 * @param size Size of the memory block to allocate in bytes
 * @return Pointer to the allocated memory, or NULL on failure
 */
void *kmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    /* Initialize heap if needed */
    if (!heap_initialized) {
        kmalloc_init();
    }

    /* Align size to 16 bytes */
    size = (size + 15) & ~15;

    /* Calculate total size needed */
    size_t total_size = size + sizeof(alloc_header_t);

    /* Find a free block */
    alloc_header_t *block = find_free_block(total_size);
    if (!block) {
        kerr("MM: Failed to allocate %zu bytes (out of memory)\n", size);
        return NULL;
    }

    /* Split the block if it's much larger than needed */
    split_block(block, total_size);

    /* Update heap usage */
    heap_used += block->size;

    /* Return a pointer to the data area */
    return (void *)((uintptr_t)block + sizeof(alloc_header_t));
}

/**
 * Allocate memory from the kernel heap and zero it
 * @param size Size of the memory block to allocate in bytes
 * @return Pointer to the allocated memory, or NULL on failure
 */
void *kzalloc(size_t size) {
    void *ptr = kmalloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

/**
 * Validate a block header
 * @param header Block header to validate
 * @return true if valid, false otherwise
 */
static bool validate_block(alloc_header_t *header) {
    /* Check if header is within the heap */
    if ((uintptr_t)header < (uintptr_t)kernel_heap ||
        (uintptr_t)header >= (uintptr_t)(kernel_heap + KERNEL_HEAP_SIZE)) {
        return false;
    }

    /* Check magic number */
    if (header->magic != ALLOC_MAGIC) {
        return false;
    }

    /* Check if size is reasonable */
    if (header->size < sizeof(alloc_header_t) ||
        (uintptr_t)header + header->size > (uintptr_t)(kernel_heap + KERNEL_HEAP_SIZE)) {
        return false;
    }

    return true;
}

/**
 * Merge adjacent free blocks
 * @param block Starting block to check for merging
 */
static void merge_blocks(alloc_header_t *block) {
    alloc_header_t *next_block = (alloc_header_t *)((uintptr_t)block + block->size);
    alloc_header_t *heap_end = (alloc_header_t *)(kernel_heap + KERNEL_HEAP_SIZE);

    /* Check if there's another block after this one */
    if ((uintptr_t)next_block >= (uintptr_t)heap_end) {
        return;
    }

    /* Validate the next block */
    if (!validate_block(next_block)) {
        return;
    }

    /* If the next block is free, merge with it */
    if (!next_block->used) {
        block->size += next_block->size;
        /* Corrupt the merged block's magic to prevent use */
        next_block->magic = 0;
    }
}

/**
 * Free a memory block allocated with kmalloc
 * @param ptr Pointer to the memory block to free
 */
void kfree(void *ptr) {
    if (!ptr || !heap_initialized) {
        return;
    }

    /* Get the block header */
    alloc_header_t *header = (alloc_header_t *)((uintptr_t)ptr - sizeof(alloc_header_t));

    /* Validate the block */
    if (!validate_block(header)) {
        kerr("MM: Attempt to free invalid memory at 0x%p\n", ptr);
        return;
    }

    /* Check if the block is already free */
    if (!header->used) {
        kerr("MM: Double free detected at 0x%p\n", ptr);
        return;
    }

    /* Mark the block as free */
    header->used = false;

    /* Update heap usage */
    heap_used -= header->size;

    /* Try to merge with the next block */
    merge_blocks(header);
}

/**
 * Resize a memory block allocated with kmalloc
 * @param ptr Pointer to the memory block to resize
 * @param size New size in bytes
 * @return Pointer to the resized memory block, or NULL on failure
 */
void *krealloc(void *ptr, size_t size) {
    /* Special cases */
    if (!ptr) {
        return kmalloc(size);
    }

    if (size == 0) {
        kfree(ptr);
        return NULL;
    }

    /* Get the original block header */
    alloc_header_t *header = (alloc_header_t *)((uintptr_t)ptr - sizeof(alloc_header_t));

    /* Validate the block */
    if (!validate_block(header)) {
        kerr("MM: Attempt to realloc invalid memory at 0x%p\n", ptr);
        return NULL;
    }

    /* Calculate the current data size */
    size_t curr_size = header->size - sizeof(alloc_header_t);

    /* If the new size is smaller, we can just return the same block */
    if (size <= curr_size) {
        return ptr;
    }

    /* Allocate a new block */
    void *new_ptr = kmalloc(size);
    if (!new_ptr) {
        return NULL;
    }

    /* Copy the data from the old block */
    memcpy(new_ptr, ptr, curr_size);

    /* Free the old block */
    kfree(ptr);

    return new_ptr;
}

/**
 * Get statistics about the kernel heap
 * @param total_mem Total memory in the heap (output)
 * @param used_mem Used memory in the heap (output)
 * @param free_mem Free memory in the heap (output)
 * @return 0 on success, negative on error
 */
int kmalloc_stats(size_t *total_mem, size_t *used_mem, size_t *free_mem) {
    if (!heap_initialized) {
        return -1;
    }

    if (total_mem) {
        *total_mem = KERNEL_HEAP_SIZE;
    }

    if (used_mem) {
        *used_mem = heap_used;
    }

    if (free_mem) {
        *free_mem = KERNEL_HEAP_SIZE - heap_used;
    }

    return 0;
}