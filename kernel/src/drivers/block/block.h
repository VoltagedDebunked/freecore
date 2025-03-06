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

#ifndef _DRIVERS_BLOCK_BLOCK_H
#define _DRIVERS_BLOCK_BLOCK_H

#include <stdint.h>
#include <stddef.h>

struct block_device;

/* Block device operations */
typedef struct block_device_ops {
    int (*read)(struct block_device *device, uint64_t offset, size_t size, void *buffer);
    int (*write)(struct block_device *device, uint64_t offset, size_t size, const void *buffer);
    int (*ioctl)(struct block_device *device, unsigned int cmd, void *arg);
} block_device_ops_t;

/* Block device structure */
typedef struct block_device {
    char name[32];             /* Device name */
    uint64_t size;             /* Total size in bytes */
    uint32_t block_size;       /* Block size in bytes */
    void *private_data;        /* Device-specific data */
    block_device_ops_t *ops;   /* Block device operations */
} block_device_t;

#endif /* _DRIVERS_BLOCK_BLOCK_H */