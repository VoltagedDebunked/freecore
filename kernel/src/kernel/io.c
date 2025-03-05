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
#include <stdarg.h>
#include <stdbool.h>
#include "io.h"
#include "config.h"

#ifdef __x86_64__
#include "../arch/x86/include/asm/serial.h"
#endif

/* Flag indicating if I/O subsystem is initialized */
static bool io_initialized = false;

/**
 * Initialize the I/O subsystem
 */
void io_init(void) {
    if (io_initialized) return;
    
#ifdef __x86_64__
    /* Initialize serial port for debugging */
    serial_init(DEBUG_SERIAL_PORT, DEBUG_SERIAL_BAUD);
    
    /* Send a test message */
    serial_write_string(DEBUG_SERIAL_PORT, "\r\n[FreeCore] Serial port initialized\r\n");
#endif
    
    io_initialized = true;
}

/**
 * Write a character to all output devices
 * @param c Character to write
 */
static void kputchar(char c) {
#ifdef __x86_64__
    /* Output to serial port */
    serial_write_char(DEBUG_SERIAL_PORT, c);
    
    /* Convert \n to \r\n for terminals */
    if (c == '\n') {
        serial_write_char(DEBUG_SERIAL_PORT, '\r');
    }
#endif
    /* Add other output devices here as needed */
}

/**
 * Write a string to all output devices
 * @param str String to write
 */
static void kputs(const char *str) {
    if (!str) return;
    
    for (size_t i = 0; str[i] != '\0'; i++) {
        kputchar(str[i]);
    }
}

/**
 * Format and print a string (printf-like)
 * @param fmt Format string
 * @param ... Arguments
 */
void kprintf(const char *fmt, ...) {
    if (!io_initialized) io_init();
    
    char buffer[1024]; /* Static buffer for formatted output */
    va_list args;
    
    va_start(args, fmt);
    kvsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    
    kputs(buffer);
}

/**
 * Print a debug message
 * @param fmt Format string
 * @param ... Arguments
 */
void kdbg(const char *fmt, ...) {
    if (!io_initialized) io_init();
    
    char buffer[1024]; /* Static buffer for formatted output */
    va_list args;
    
    /* Print debug prefix */
    kputs("[DEBUG] ");
    
    va_start(args, fmt);
    kvsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    
    kputs(buffer);
}

/**
 * Print an error message
 * @param fmt Format string
 * @param ... Arguments
 */
void kerr(const char *fmt, ...) {
    if (!io_initialized) io_init();
    
    char buffer[1024]; /* Static buffer for formatted output */
    va_list args;
    
    /* Print error prefix */
    kputs("[ERROR] ");
    
    va_start(args, fmt);
    kvsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    
    kputs(buffer);
}

/**
 * Format a string into a buffer (snprintf-like)
 * @param buf Output buffer
 * @param size Buffer size
 * @param fmt Format string
 * @param ... Arguments
 * @return Number of characters written (excluding null terminator)
 */
int ksnprintf(char *buf, size_t size, const char *fmt, ...) {
    va_list args;
    int ret;
    
    va_start(args, fmt);
    ret = kvsnprintf(buf, size, fmt, args);
    va_end(args);
    
    return ret;
}

/**
 * Convert an integer to a string with the given base
 * @param value Value to convert
 * @param base Base for conversion (2-16)
 * @param is_signed Whether to treat value as signed
 * @param width Minimum field width
 * @param pad_char Character to use for padding
 * @param buf Output buffer
 * @param size Buffer size
 * @return Number of characters written
 */
static int int_to_string(uint64_t value, int base, bool is_signed, int width, 
                         char pad_char, char *buf, size_t size) {
    static const char digits[] = "0123456789ABCDEF";
    char tmp[65]; /* Enough for 64-bit integers in binary plus sign */
    char *tmp_ptr = tmp + sizeof(tmp) - 1;
    int len = 0;
    bool negative = false;
    int64_t signed_value = (int64_t)value;
    
    /* Handle invalid base */
    if (base < 2 || base > 16) {
        return 0;
    }
    
    /* Ensure we don't write past the end of the buffer */
    if (size == 0) return 0;
    
    /* Handle sign for signed numbers */
    if (is_signed && signed_value < 0) {
        negative = true;
        value = -signed_value;
    } else {
        value = (uint64_t)signed_value;
    }
    
    /* Null terminate the temporary buffer */
    *tmp_ptr = '\0';
    tmp_ptr--;
    
    /* Handle special case for 0 */
    if (value == 0) {
        *tmp_ptr = '0';
        tmp_ptr--;
        len = 1;
    } else {
        /* Convert to string in reverse order */
        while (value > 0) {
            *tmp_ptr = digits[value % base];
            value /= base;
            tmp_ptr--;
            len++;
        }
    }
    
    /* Add sign if needed */
    if (negative) {
        *tmp_ptr = '-';
        tmp_ptr--;
        len++;
    }
    
    /* Add padding if needed */
    while (len < width) {
        *tmp_ptr = pad_char;
        tmp_ptr--;
        len++;
    }
    
    /* Copy to output buffer, respecting size limit */
    size_t out_len = 0;
    tmp_ptr++; /* Point to first character */
    
    while (*tmp_ptr && out_len < size - 1) {
        buf[out_len++] = *tmp_ptr++;
    }
    
    /* Null terminate output buffer */
    buf[out_len] = '\0';
    
    return out_len;
}

/**
 * Format a string (vsnprintf-like)
 * @param buf Output buffer
 * @param size Buffer size
 * @param fmt Format string
 * @param args Variable arguments list
 * @return Number of characters written (excluding null terminator)
 */
int kvsnprintf(char *buf, size_t size, const char *fmt, va_list args) {
    if (!buf || size == 0) return 0;
    
    size_t out_idx = 0;
    
    /* Process format string */
    for (size_t i = 0; fmt[i] != '\0' && out_idx < size - 1; i++) {
        /* Handle format specifiers */
        if (fmt[i] == '%') {
            int width = 0;
            char pad_char = ' ';
            bool long_flag = false;
            
            i++; /* Move past '%' */
            
            /* Handle padding specifier */
            if (fmt[i] == '0') {
                pad_char = '0';
                i++;
            }
            
            /* Handle width specifier */
            while (fmt[i] >= '0' && fmt[i] <= '9') {
                width = width * 10 + (fmt[i] - '0');
                i++;
            }
            
            /* Handle length modifier */
            if (fmt[i] == 'l') {
                long_flag = true;
                i++;
                if (fmt[i] == 'l') {
                    /* Handle 'll' for long long */
                    i++;
                }
            }
            
            /* Handle format specifier */
            switch (fmt[i]) {
                case 'c': {
                    /* Character */
                    char c = (char)va_arg(args, int);
                    if (out_idx < size - 1) {
                        buf[out_idx++] = c;
                    }
                    break;
                }
                
                case 's': {
                    /* String */
                    const char *str = va_arg(args, const char *);
                    if (!str) str = "(null)";
                    
                    while (*str && out_idx < size - 1) {
                        buf[out_idx++] = *str++;
                    }
                    break;
                }
                
                case 'd':
                case 'i': {
                    /* Signed integer */
                    int64_t value;
                    if (long_flag) {
                        value = va_arg(args, int64_t);
                    } else {
                        value = va_arg(args, int);
                    }
                    
                    char tmp[24]; /* Large enough for 64-bit integers */
                    int len = int_to_string(value, 10, true, width, pad_char, tmp, sizeof(tmp));
                    
                    for (int j = 0; j < len && out_idx < size - 1; j++) {
                        buf[out_idx++] = tmp[j];
                    }
                    break;
                }
                
                case 'u': {
                    /* Unsigned integer */
                    uint64_t value;
                    if (long_flag) {
                        value = va_arg(args, uint64_t);
                    } else {
                        value = va_arg(args, unsigned int);
                    }
                    
                    char tmp[24]; /* Large enough for 64-bit integers */
                    int len = int_to_string(value, 10, false, width, pad_char, tmp, sizeof(tmp));
                    
                    for (int j = 0; j < len && out_idx < size - 1; j++) {
                        buf[out_idx++] = tmp[j];
                    }
                    break;
                }
                
                case 'x':
                case 'X': {
                    /* Hexadecimal */
                    uint64_t value;
                    if (long_flag) {
                        value = va_arg(args, uint64_t);
                    } else {
                        value = va_arg(args, unsigned int);
                    }
                    
                    char tmp[24]; /* Large enough for 64-bit integers */
                    int len = int_to_string(value, 16, false, width, pad_char, tmp, sizeof(tmp));
                    
                    for (int j = 0; j < len && out_idx < size - 1; j++) {
                        buf[out_idx++] = tmp[j];
                    }
                    break;
                }
                
                case 'p': {
                    /* Pointer */
                    void *ptr = va_arg(args, void *);
                    
                    /* Write 0x prefix */
                    if (out_idx < size - 1) buf[out_idx++] = '0';
                    if (out_idx < size - 1) buf[out_idx++] = 'x';
                    
                    char tmp[24]; /* Large enough for 64-bit pointers */
                    int len = int_to_string((uint64_t)ptr, 16, false, 
                                            sizeof(void*) * 2, '0', tmp, sizeof(tmp));
                    
                    for (int j = 0; j < len && out_idx < size - 1; j++) {
                        buf[out_idx++] = tmp[j];
                    }
                    break;
                }
                
                case '%': {
                    /* Literal '%' */
                    if (out_idx < size - 1) {
                        buf[out_idx++] = '%';
                    }
                    break;
                }
                
                default: {
                    /* Unrecognized format specifier, just output it */
                    if (out_idx < size - 1) {
                        buf[out_idx++] = '%';
                    }
                    if (out_idx < size - 1) {
                        buf[out_idx++] = fmt[i];
                    }
                    break;
                }
            }
        } else {
            /* Regular character */
            buf[out_idx++] = fmt[i];
        }
    }
    
    /* Null terminate the output */
    buf[out_idx] = '\0';
    
    return out_idx;
}