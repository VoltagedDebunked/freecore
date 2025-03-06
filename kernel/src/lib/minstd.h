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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _MINSTD_H
#define _MINSTD_H

#include <stddef.h>

/*
 * Memory manipulation functions with safety checks
 * - All functions check for NULL pointers and report errors
 * - memcpy detects and handles overlapping memory regions
 * - memmove properly handles overlapping regions in either direction
 */
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

/*
 * String functions with safety checks
 * - All functions check for NULL pointers and report errors
 */
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
size_t strnlen(const char *s, size_t maxlen);
size_t strlen(const char *s);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
char *strcat(char *dest, const char *src);
char *strrchr(const char *s, int c);
char *strtok_r(char *str, const char *delim, char **saveptr);
size_t strspn(const char *s, const char *accept);
size_t strcspn(const char *s, const char *reject);

#endif /* _MINSTD_H */