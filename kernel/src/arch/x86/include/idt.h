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

#ifndef _ASM_X86_IDT_H
#define _ASM_X86_IDT_H

#include <stdint.h>

/* IDT entry flags */
#define IDT_FLAGS_PRESENT        0x80    /* Interrupt is present in memory */
#define IDT_FLAGS_INTERRUPT_GATE 0x0E    /* 64-bit interrupt gate */
#define IDT_FLAGS_TRAP_GATE      0x0F    /* 64-bit trap gate */
#define IDT_FLAGS_RING0          0x00    /* Ring 0 (kernel) privilege */
#define IDT_FLAGS_RING3          0x60    /* Ring 3 (user) privilege */

/* Total number of interrupt vectors */
#define IDT_VECTOR_COUNT 256

/* IDT entry structure for 64-bit mode */
struct idt_entry {
    uint16_t offset_low;       /* Offset bits 0-15 */
    uint16_t selector;         /* Selector (usually kernel code segment) */
    uint8_t  ist;              /* Interrupt Stack Table (IST) index */
    uint8_t  flags;            /* Gate type, privilege level, present flag */
    uint16_t offset_mid;       /* Offset bits 16-31 */
    uint32_t offset_high;      /* Offset bits 32-63 */
    uint32_t reserved;         /* Reserved, must be zero */
} __attribute__((packed));

/* IDT pointer structure for LIDT instruction */
struct idt_ptr {
    uint16_t limit;            /* Size of IDT minus 1 */
    uint64_t base;             /* Base address of IDT */
} __attribute__((packed));

/* Function prototypes */
void idt_init(void);
void idt_load(struct idt_ptr *idt_ptr_addr);

/* Exception handler function type */
typedef void (*exception_handler)(void);

/* Register an exception handler for a specific vector */
void idt_register_handler(uint8_t vector, exception_handler handler);

#endif /* _ASM_X86_IDT_H */