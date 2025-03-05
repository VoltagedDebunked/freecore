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
#include <arch/x86/include/serial.h>

/* I/O Port Functions */
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * Initialize a serial port
 * @param port The serial port base address
 * @param baud_divisor The baud rate divisor
 */
void serial_init(uint16_t port, uint16_t baud_divisor) {
    /* Disable interrupts */
    outb(port + SERIAL_INTR_ENABLE_REG, 0x00);
    
    /* Set DLAB to access divisor latches */
    outb(port + SERIAL_LINE_CTRL_REG, SERIAL_LCR_DLAB);
    
    /* Set baud rate divisor */
    outb(port + SERIAL_DATA_REG, baud_divisor & 0xFF);               /* Low byte */
    outb(port + SERIAL_DATA_REG + 1, (baud_divisor >> 8) & 0xFF);    /* High byte */
    
    /* 8 bits, 1 stop bit, no parity */
    outb(port + SERIAL_LINE_CTRL_REG, SERIAL_LCR_8BITS | SERIAL_LCR_1STOP);
    
    /* Enable and configure FIFO */
    outb(port + SERIAL_FIFO_CTRL_REG, SERIAL_FCR_ENABLE | SERIAL_FCR_CLEAR_RX | SERIAL_FCR_CLEAR_TX | SERIAL_FCR_TRIGGER_14);
    
    /* Set DTR, RTS, and OUT2 */
    outb(port + SERIAL_MODEM_CTRL_REG, SERIAL_MCR_DTR | SERIAL_MCR_RTS | SERIAL_MCR_OUT2);
}

/**
 * Check if the transmit buffer is empty
 * @param port The serial port base address
 * @return 1 if buffer is empty, 0 otherwise
 */
int serial_is_transmit_empty(uint16_t port) {
    return inb(port + SERIAL_LINE_STATUS_REG) & SERIAL_LSR_TX_READY;
}

/**
 * Write a character to the serial port
 * @param port The serial port base address
 * @param c The character to write
 */
void serial_write_char(uint16_t port, char c) {
    /* Wait until transmit buffer is empty */
    while (serial_is_transmit_empty(port) == 0);
    
    /* Send the character */
    outb(port + SERIAL_DATA_REG, c);
}

/**
 * Check if data is available to read
 * @param port The serial port base address
 * @return 1 if data available, 0 otherwise
 */
int serial_received(uint16_t port) {
    return inb(port + SERIAL_LINE_STATUS_REG) & SERIAL_LSR_RX_READY;
}

/**
 * Read a character from the serial port
 * @param port The serial port base address
 * @return The character read
 */
char serial_read_char(uint16_t port) {
    /* Wait until data is available */
    while (serial_received(port) == 0);
    
    /* Read the character */
    return inb(port + SERIAL_DATA_REG);
}

/**
 * Write a string to the serial port
 * @param port The serial port base address
 * @param str The string to write
 */
void serial_write_string(uint16_t port, const char *str) {
    if (!str) return;
    
    /* Write each character */
    for (size_t i = 0; str[i] != '\0'; i++) {
        serial_write_char(port, str[i]);
    }
}

/**
 * Write a hexadecimal value to the serial port
 * @param port The serial port base address
 * @param value The value to write
 * @param num_digits Number of hex digits to display (1-16)
 */
void serial_write_hex(uint16_t port, uint64_t value, int num_digits) {
    static const char hex_chars[] = "0123456789ABCDEF";
    
    /* Clamp num_digits to valid range */
    if (num_digits < 1) num_digits = 1;
    if (num_digits > 16) num_digits = 16;
    
    /* Write "0x" prefix */
    serial_write_string(port, "0x");
    
    /* Write each digit from most significant to least significant */
    for (int i = num_digits - 1; i >= 0; i--) {
        uint8_t digit = (value >> (i * 4)) & 0xF;
        serial_write_char(port, hex_chars[digit]);
    }
}

/**
 * Write a signed integer to the serial port
 * @param port The serial port base address
 * @param value The value to write
 */
void serial_write_int(uint16_t port, int64_t value) {
    /* Handle negative numbers */
    if (value < 0) {
        serial_write_char(port, '-');
        value = -value;
    }
    
    /* Handle special case for zero */
    if (value == 0) {
        serial_write_char(port, '0');
        return;
    }
    
    /* Convert to string (backwards) */
    char buffer[24]; /* Enough for 64-bit integers */
    int idx = 0;
    
    while (value > 0) {
        buffer[idx++] = '0' + (value % 10);
        value /= 10;
    }
    
    /* Output in correct order */
    for (int i = idx - 1; i >= 0; i--) {
        serial_write_char(port, buffer[i]);
    }
}