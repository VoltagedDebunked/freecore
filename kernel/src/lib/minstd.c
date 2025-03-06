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

#include <stddef.h>
#include <stdint.h>
#include "minstd.h"
#include <kernel/io.h>

/* Standard library functions with added safety checks */
void *memcpy(void *dest, const void *src, size_t n) {
    /* Check for NULL pointers */
    if (!dest || !src) {
        kerr("memcpy: NULL pointer detected\n");
        return dest;
    }

    /* Check for zero size (no-op) */
    if (n == 0) {
        return dest;
    }

    /* Check for potential overlap */
    if ((uintptr_t)dest <= (uintptr_t)src && (uintptr_t)src < (uintptr_t)dest + n) {
        kerr("memcpy: memory regions overlap, use memmove instead\n");
        return memmove(dest, src, n); /* Fall back to memmove for safety */
    }

    if ((uintptr_t)src <= (uintptr_t)dest && (uintptr_t)dest < (uintptr_t)src + n) {
        kerr("memcpy: memory regions overlap, use memmove instead\n");
        return memmove(dest, src, n); /* Fall back to memmove for safety */
    }

    /* Perform the actual copy */
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;

    for (size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }

    return dest;
}

void *memset(void *s, int c, size_t n) {
    /* Check for NULL pointer */
    if (!s) {
        kerr("memset: NULL pointer detected\n");
        return s;
    }

    /* Check for zero size (no-op) */
    if (n == 0) {
        return s;
    }

    /* Perform the operation */
    uint8_t *p = (uint8_t *)s;
    uint8_t value = (uint8_t)c;

    for (size_t i = 0; i < n; i++) {
        p[i] = value;
    }

    return s;
}

void *memmove(void *dest, const void *src, size_t n) {
    /* Check for NULL pointers */
    if (!dest || !src) {
        kerr("memmove: NULL pointer detected\n");
        return dest;
    }

    /* Check for zero size (no-op) */
    if (n == 0) {
        return dest;
    }

    /* Handle case when dest and src are the same */
    if (dest == src) {
        return dest;
    }

    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;

    /* Check which direction to copy to handle overlap correctly */
    if (src > dest) {
        /* Copy forward */
        for (size_t i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }
    } else {
        /* Copy backward */
        for (size_t i = n; i > 0; i--) {
            pdest[i-1] = psrc[i-1];
        }
    }

    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    /* Check for NULL pointers */
    if (!s1 || !s2) {
        kerr("memcmp: NULL pointer detected\n");
        return 0; /* Safer than returning a random value */
    }

    /* Check for zero size */
    if (n == 0) {
        return 0;
    }

    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }

    return 0;
}

int strcmp(const char *s1, const char *s2) {
    /* Check for NULL pointers */
    if (!s1 || !s2) {
        kerr("strcmp: NULL pointer detected\n");
        return 0; /* Safer than returning a random value */
    }

    while (*s1 && *s2) {
        if (*s1 != *s2) {
            return *(const unsigned char *)s1 - *(const unsigned char *)s2;
        }
        s1++;
        s2++;
    }

    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

/* Additional safety functions */

size_t strnlen(const char *s, size_t maxlen) {
    /* Check for NULL pointer */
    if (!s) {
        kerr("strnlen: NULL pointer detected\n");
        return 0;
    }

    size_t len = 0;
    while (len < maxlen && s[len]) {
        len++;
    }

    return len;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    /* Check for NULL pointers */
    if (!s1 || !s2) {
        kerr("strncmp: NULL pointer detected\n");
        return 0;
    }

    /* Zero-length comparison is always equal */
    if (n == 0) {
        return 0;
    }

    while (n > 1 && *s1 && *s2) {
        if (*s1 != *s2) {
            break;
        }
        s1++;
        s2++;
        n--;
    }

    return (n > 0) ? *(const unsigned char *)s1 - *(const unsigned char *)s2 : 0;
}

size_t strlen(const char *s) {
    /* Check for NULL pointer */
    if (!s) {
        kerr("strlen: NULL pointer detected\n");
        return 0;
    }

    const char *p = s;
    while (*p) {
        p++;
    }
    return p - s;
}

char *strcpy(char *dest, const char *src) {
    if (!dest || !src) {
        kerr("strcpy: NULL pointer detected\n");
        return dest;
    }

    char *original_dest = dest;
    while ((*dest++ = *src++) != '\0');
    return original_dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
    if (!dest || !src) {
        kerr("strncpy: NULL pointer detected\n");
        return dest;
    }

    char *original_dest = dest;
    size_t i;

    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }

    for (; i < n; i++) {
        dest[i] = '\0';
    }

    return original_dest;
}

char *strcat(char *dest, const char *src) {
    if (!dest || !src) {
        kerr("strcat: NULL pointer detected\n");
        return dest;
    }

    char *original_dest = dest;

    while (*dest) {
        dest++;
    }

    while ((*dest++ = *src++) != '\0');

    return original_dest;
}

char *strrchr(const char *s, int c) {
    if (!s) {
        kerr("strrchr: NULL pointer detected\n");
        return NULL;
    }

    const char *last_occurrence = NULL;

    while (*s) {
        if (*s == (char)c) {
            last_occurrence = s;
        }
        s++;
    }

    if (*s == (char)c) {
        last_occurrence = s;
    }

    return (char *)last_occurrence;
}

char *strtok_r(char *str, const char *delim, char **saveptr) {
    char *token_start;
    char *token_end;

    if (!saveptr) {
        kerr("strtok_r: NULL saveptr\n");
        return NULL;
    }

    if (str == NULL) {
        str = *saveptr;
    }

    if (!str) {
        return NULL;
    }

    str += strspn(str, delim);
    if (*str == '\0') {
        *saveptr = NULL;
        return NULL;
    }

    token_start = str;
    token_end = str + strcspn(str, delim);

    if (*token_end == '\0') {
        *saveptr = NULL;
    } else {
        *token_end = '\0';
        *saveptr = token_end + 1;
    }

    return token_start;
}

size_t strspn(const char *s, const char *accept) {
    const char *p;
    const char *a;
    size_t count = 0;

    if (!s || !accept) {
        kerr("strspn: NULL pointer detected\n");
        return 0;
    }

    for (p = s; *p; p++) {
        for (a = accept; *a; a++) {
            if (*p == *a)
                break;
        }
        if (*a == '\0')
            return count;
        count++;
    }

    return count;
}

size_t strcspn(const char *s, const char *reject) {
    const char *p;
    const char *a;

    if (!s || !reject) {
        kerr("strcspn: NULL pointer detected\n");
        return 0;
    }

    for (p = s; *p; p++) {
        for (a = reject; *a; a++) {
            if (*p == *a)
                return p - s;
        }
    }

    return p - s;
}