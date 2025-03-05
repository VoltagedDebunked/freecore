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

#ifndef _ASM_X86_SERIAL_H
#define _ASM_X86_SERIAL_H

#include <stdint.h>

/* Standard COM port addresses */
#define COM1_PORT 0x3F8
#define COM2_PORT 0x2F8
#define COM3_PORT 0x3E8
#define COM4_PORT 0x2E8

/* Serial port registers (offset from base port) */
#define SERIAL_DATA_REG          0x0  /* Data register (R/W) */
#define SERIAL_INTR_ENABLE_REG   0x1  /* Interrupt enable register (W) */
#define SERIAL_FIFO_CTRL_REG     0x2  /* FIFO control register (W) */
#define SERIAL_LINE_CTRL_REG     0x3  /* Line control register (W) */
#define SERIAL_MODEM_CTRL_REG    0x4  /* Modem control register (W) */
#define SERIAL_LINE_STATUS_REG   0x5  /* Line status register (R) */
#define SERIAL_MODEM_STATUS_REG  0x6  /* Modem status register (R) */
#define SERIAL_SCRATCH_REG       0x7  /* Scratch register (R/W) */

/* Line status register bits */
#define SERIAL_LSR_RX_READY      0x01  /* Data ready to be read */
#define SERIAL_LSR_TX_READY      0x20  /* Transmitter ready to send */

/* FIFO control register bits */
#define SERIAL_FCR_ENABLE        0x01  /* Enable FIFO */
#define SERIAL_FCR_CLEAR_RX      0x02  /* Clear receive FIFO */
#define SERIAL_FCR_CLEAR_TX      0x04  /* Clear transmit FIFO */
#define SERIAL_FCR_TRIGGER_14    0xC0  /* 14 bytes trigger level */

/* Line control register bits */
#define SERIAL_LCR_8BITS         0x03  /* 8 bits data length */
#define SERIAL_LCR_1STOP         0x00  /* 1 stop bit */
#define SERIAL_LCR_DLAB          0x80  /* Divisor latch access bit */

/* Modem control register bits */
#define SERIAL_MCR_DTR           0x01  /* Data Terminal Ready */
#define SERIAL_MCR_RTS           0x02  /* Request To Send */
#define SERIAL_MCR_OUT1          0x04  /* Output 1 */
#define SERIAL_MCR_OUT2          0x08  /* Output 2 (enables interrupts) */
#define SERIAL_MCR_LOOPBACK      0x10  /* Enable loopback mode */

/* Baud rates divisors */
#define SERIAL_BAUD_115200       1     /* 115200 baud divisor */
#define SERIAL_BAUD_57600        2     /* 57600 baud divisor */
#define SERIAL_BAUD_38400        3     /* 38400 baud divisor */
#define SERIAL_BAUD_19200        6     /* 19200 baud divisor */
#define SERIAL_BAUD_9600         12    /* 9600 baud divisor */
#define SERIAL_BAUD_4800         24    /* 4800 baud divisor */
#define SERIAL_BAUD_2400         48    /* 2400 baud divisor */

/* Function prototypes */
void serial_init(uint16_t port, uint16_t baud_divisor);
void serial_write_char(uint16_t port, char c);
char serial_read_char(uint16_t port);
int serial_is_transmit_empty(uint16_t port);
int serial_received(uint16_t port);
void serial_write_string(uint16_t port, const char *str);
void serial_write_hex(uint16_t port, uint64_t value, int num_digits);
void serial_write_int(uint16_t port, int64_t value);

#endif /* _ASM_X86_SERIAL_H */