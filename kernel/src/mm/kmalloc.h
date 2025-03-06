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

#ifndef _MM_KMALLOC_H
#define _MM_KMALLOC_H

#include <stddef.h>
#include <stdint.h>

/**
 * Allocate memory from the kernel heap
 * @param size Size of the memory block to allocate in bytes
 * @return Pointer to the allocated memory, or NULL on failure
 */
void *kmalloc(size_t size);

/**
 * Allocate memory from the kernel heap and zero it
 * @param size Size of the memory block to allocate in bytes
 * @return Pointer to the allocated memory, or NULL on failure
 */
void *kzalloc(size_t size);

/**
 * Allocate memory from the kernel heap with a specific alignment
 * @param size Size of the memory block to allocate in bytes
 * @param align Alignment requirement (must be a power of 2)
 * @return Pointer to the allocated memory, or NULL on failure
 */
void *kmalloc_aligned(size_t size, size_t align);

/**
 * Free a memory block allocated with kmalloc
 * @param ptr Pointer to the memory block to free
 */
void kfree(void *ptr);

/**
 * Resize a memory block allocated with kmalloc
 * @param ptr Pointer to the memory block to resize
 * @param size New size in bytes
 * @return Pointer to the resized memory block, or NULL on failure
 */
void *krealloc(void *ptr, size_t size);

/**
 * Initialize the kernel heap
 * @return 0 on success, negative on error
 */
int kmalloc_init(void);

/**
 * Get statistics about the kernel heap
 * @param total_mem Total memory in the heap (output)
 * @param used_mem Used memory in the heap (output)
 * @param free_mem Free memory in the heap (output)
 * @return 0 on success, negative on error
 */
int kmalloc_stats(size_t *total_mem, size_t *used_mem, size_t *free_mem);

#endif /* _MM_KMALLOC_H */