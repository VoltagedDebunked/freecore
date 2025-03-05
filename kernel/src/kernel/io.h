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

#ifndef _KERNEL_IO_H
#define _KERNEL_IO_H

#include <stdint.h>
#include <stdarg.h>

/**
 * Initialize kernel I/O subsystem
 */
void io_init(void);

/**
 * Print a formatted string (printf-like)
 * @param fmt Format string
 * @param ... Arguments
 */
void kprintf(const char *fmt, ...);

/**
 * Print a formatted string to the debug output
 * @param fmt Format string
 * @param ... Arguments
 */
void kdbg(const char *fmt, ...);

/**
 * Print a formatted error message
 * @param fmt Format string
 * @param ... Arguments
 */
void kerr(const char *fmt, ...);

/**
 * Printf-style formatting into a buffer
 * @param buf Buffer to write to
 * @param size Size of buffer
 * @param fmt Format string
 * @param ... Arguments
 * @return Number of characters written (excluding null terminator)
 */
int ksnprintf(char *buf, size_t size, const char *fmt, ...);

/**
 * vprintf-style formatting into a buffer
 * @param buf Buffer to write to
 * @param size Size of buffer
 * @param fmt Format string
 * @param args Arguments
 * @return Number of characters written (excluding null terminator)
 */
int kvsnprintf(char *buf, size_t size, const char *fmt, va_list args);

#endif /* _KERNEL_IO_H */