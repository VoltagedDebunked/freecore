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

#ifndef _KERNEL_CONFIG_H
#define _KERNEL_CONFIG_H

/* Kernel version information */
#define KERNEL_VERSION_MAJOR    0
#define KERNEL_VERSION_MINOR    1
#define KERNEL_VERSION_PATCH    0
#define KERNEL_VERSION_STRING   "0.1.0"

/* Debug output configuration */
#define DEBUG_ENABLED           1
#define DEBUG_SERIAL_PORT       COM1_PORT
#define DEBUG_SERIAL_BAUD       SERIAL_BAUD_115200

/* Memory configuration */
#define PAGE_SIZE               4096

/* Kernel stack size (bytes) */
#define KERNEL_STACK_SIZE       16384  /* 16 KiB */

/* Maximum number of CPUs supported */
#define MAX_CPUS                64

#endif /* _KERNEL_CONFIG_H */