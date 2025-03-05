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

#ifndef _ASM_X86_GDT_H
#define _ASM_X86_GDT_H

#include <stdint.h>

/* GDT entry flags (granularity byte) */
#define GDT_FLAG_GRANULARITY   0x80    /* Granularity (0 = 1B, 1 = 4KB) */
#define GDT_FLAG_SIZE          0x40    /* Size (0 = 16-bit, 1 = 32-bit) */
#define GDT_FLAG_LONG_MODE     0x20    /* Long mode (64-bit segment) */
#define GDT_FLAG_AVAILABLE     0x10    /* Available for system use */

/* GDT entry access flags */
#define GDT_ACCESS_PRESENT     0x80    /* Present in memory */
#define GDT_ACCESS_RING0       0x00    /* Ring 0 (highest privilege) */
#define GDT_ACCESS_RING1       0x20    /* Ring 1 */
#define GDT_ACCESS_RING2       0x40    /* Ring 2 */
#define GDT_ACCESS_RING3       0x60    /* Ring 3 (lowest privilege) */
#define GDT_ACCESS_SYSTEM      0x10    /* System segment */
#define GDT_ACCESS_EXECUTABLE  0x08    /* Executable segment */
#define GDT_ACCESS_DC          0x04    /* Direction/Conforming bit */
#define GDT_ACCESS_RW          0x02    /* Readable / Writable */
#define GDT_ACCESS_ACCESSED    0x01    /* Accessed (set by CPU) */

/* TSS access byte bits */
#define GDT_ACCESS_TSS         0x09    /* 64-bit TSS */

/* Segment selector values */
#define GDT_NULL               0       /* NULL descriptor */
#define GDT_KERNEL_CODE        1       /* Kernel code segment */
#define GDT_KERNEL_DATA        2       /* Kernel data segment */
#define GDT_USER_CODE          3       /* User code segment */
#define GDT_USER_DATA          4       /* User data segment */
#define GDT_TSS                5       /* TSS segment */

/* Segment selector calculation:
 * Selector = (Index << 3) | TI | RPL
 * Where:
 * - Index is the GDT entry index (0-based)
 * - TI (Table Indicator) is 0 for GDT, 1 for LDT (always 0 here)
 * - RPL (Requested Privilege Level) is 0 for kernel
 */
#define GDT_KERNEL_CODE_SELECTOR  ((GDT_KERNEL_CODE << 3) | 0)
#define GDT_KERNEL_DATA_SELECTOR  ((GDT_KERNEL_DATA << 3) | 0)
#define GDT_USER_CODE_SELECTOR    ((GDT_USER_CODE << 3) | 3)
#define GDT_USER_DATA_SELECTOR    ((GDT_USER_DATA << 3) | 3)
#define GDT_TSS_SELECTOR          ((GDT_TSS << 3) | 0)

/* GDT pointer structure */
struct gdt_ptr {
    uint16_t limit;           /* Size of GDT minus 1 */
    uint64_t base;            /* Base address of GDT */
} __attribute__((packed));

/* Function prototypes */
void gdt_init(void);
void gdt_load(struct gdt_ptr *gdt_ptr_addr);
void tss_load(uint16_t selector);

/* Function to set kernel stack in TSS */
void gdt_set_kernel_stack(uint64_t stack);

#endif /* _ASM_X86_GDT_H */