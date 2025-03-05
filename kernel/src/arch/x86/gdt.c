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
#include <lib/minstd.h>
#include <arch/x86/include/gdt.h>
#include <kernel/io.h>

/* 
 * GDT structure for x86_64
 * We need at least:
 * - NULL descriptor
 * - Kernel code segment (64-bit)
 * - Kernel data segment
 * - User code segment (64-bit)
 * - User data segment
 * - TSS descriptor (for privilege level transitions)
 */

#define GDT_ENTRIES 6

/* 64-bit GDT entry structure */
struct gdt_entry_64 {
    uint16_t limit_low;       /* Limit (bits 0-15) */
    uint16_t base_low;        /* Base (bits 0-15) */
    uint8_t  base_middle;     /* Base (bits 16-23) */
    uint8_t  access;          /* Access byte */
    uint8_t  granularity;     /* Granularity byte */
    uint8_t  base_high;       /* Base (bits 24-31) */
};

/* 64-bit TSS entry structure (16 bytes) */
struct tss_entry_64 {
    uint16_t length_low;      /* Length (bits 0-15) */
    uint16_t base_low;        /* Base (bits 0-15) */
    uint8_t  base_middle1;    /* Base (bits 16-23) */
    uint8_t  access;          /* Access byte (type + present) */
    uint8_t  granularity;     /* Granularity (limit + flags) */
    uint8_t  base_middle2;    /* Base (bits 24-31) */
    uint32_t base_high;       /* Base (bits 32-63) */
    uint32_t reserved;        /* Reserved */
};

/* TSS structure for 64-bit mode */
struct tss_struct {
    uint32_t reserved0;
    uint64_t rsp0;           /* Stack pointer for CPL 0 */
    uint64_t rsp1;           /* Stack pointer for CPL 1 */
    uint64_t rsp2;           /* Stack pointer for CPL 2 */
    uint64_t reserved1;
    uint64_t ist[7];         /* Interrupt Stack Table (IST) entries */
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;     /* I/O Map Base Address */
} __attribute__((packed));

/* Combined structure to define the GDT in memory */
struct gdt_full {
    struct gdt_entry_64 entries[GDT_ENTRIES - 1]; /* Regular entries */
    struct tss_entry_64 tss_entry;                /* TSS entry (16 bytes) */
} __attribute__((packed));

/* Allocate GDT structure in memory */
static struct gdt_full gdt;

/* TSS structure */
static struct tss_struct tss = {0};

/* GDT pointer structure for LGDT instruction */
static struct gdt_ptr gdt_ptr;

/* Set a GDT entry */
static void gdt_set_entry(int idx, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity) {
    /* Set standard entry fields */
    gdt.entries[idx].base_low = base & 0xFFFF;
    gdt.entries[idx].base_middle = (base >> 16) & 0xFF;
    gdt.entries[idx].base_high = (base >> 24) & 0xFF;
    
    gdt.entries[idx].limit_low = limit & 0xFFFF;
    gdt.entries[idx].granularity = ((limit >> 16) & 0x0F) | (granularity & 0xF0);
    
    gdt.entries[idx].access = access;
}

/* Set the 64-bit TSS entry */
static void gdt_set_tss(uint64_t base, uint32_t limit) {
    /* Set the lower part (standard descriptor) */
    gdt.tss_entry.base_low = base & 0xFFFF;
    gdt.tss_entry.base_middle1 = (base >> 16) & 0xFF;
    gdt.tss_entry.base_middle2 = (base >> 24) & 0xFF;
    gdt.tss_entry.base_high = (base >> 32) & 0xFFFFFFFF;
    
    gdt.tss_entry.length_low = limit & 0xFFFF;
    gdt.tss_entry.granularity = ((limit >> 16) & 0x0F);
    
    /* TSS present, type = 0x9 (64-bit TSS) */
    gdt.tss_entry.access = 0x89;
    
    gdt.tss_entry.reserved = 0;
}

/* Initialize TSS */
static void tss_init(void) {
    /* Clear the TSS structure */
    memset(&tss, 0, sizeof(tss));
    
    /* Set up the kernel stack pointer (RSP0) */
    tss.rsp0 = 0; /* This should be set to the kernel stack address */
    
    /* The I/O permission bitmap is directly after the TSS */
    tss.iomap_base = sizeof(tss);
    
    /* Set up the TSS entry in the GDT */
    gdt_set_tss((uint64_t)&tss, sizeof(tss) - 1);
}

/* Initialize the GDT */
void gdt_init(void) {
    kprintf("GDT: Initializing 64-bit GDT and TSS...\n");
    
    /* Set up the GDT pointer */
    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base = (uint64_t)&gdt;
    
    /* Clear the GDT */
    memset(&gdt, 0, sizeof(gdt));
    
    /* NULL descriptor (required) */
    gdt_set_entry(GDT_NULL, 0, 0, 0, 0);
    
    /* Kernel code segment (64-bit) */
    /* access: Present = 1, Ring = 0, Type = 1 (code/data), Execute = 1, Direction = 0, RW = 1, Accessed = 0 */
    /* granularity: Granularity = 1 (4K), Size = 0 (must be 0 in 64-bit mode), Long = 1 (64-bit), Avl = 0 */
    gdt_set_entry(GDT_KERNEL_CODE, 0, 0xFFFFF, 
                 GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_SYSTEM | 
                 GDT_ACCESS_EXECUTABLE | GDT_ACCESS_RW, 
                 GDT_FLAG_GRANULARITY | GDT_FLAG_LONG_MODE);
    
    /* Kernel data segment */
    /* Same as code except Execute = 0 */
    gdt_set_entry(GDT_KERNEL_DATA, 0, 0xFFFFF, 
                 GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_SYSTEM | GDT_ACCESS_RW, 
                 GDT_FLAG_GRANULARITY | GDT_FLAG_SIZE);
    
    /* User code segment (64-bit) */
    /* Similar to kernel code but with Ring = 3 */
    gdt_set_entry(GDT_USER_CODE, 0, 0xFFFFF, 
                 GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_SYSTEM | 
                 GDT_ACCESS_EXECUTABLE | GDT_ACCESS_RW, 
                 GDT_FLAG_GRANULARITY | GDT_FLAG_LONG_MODE);
    
    /* User data segment */
    /* Similar to kernel data but with Ring = 3 */
    gdt_set_entry(GDT_USER_DATA, 0, 0xFFFFF, 
                 GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_SYSTEM | GDT_ACCESS_RW, 
                 GDT_FLAG_GRANULARITY | GDT_FLAG_SIZE);
    
    /* Initialize the TSS */
    tss_init();
    
    /* Load the GDT */
    kprintf("GDT: Loading GDT...\n");
    gdt_load(&gdt_ptr);
    
    /* Load the TSS */
    kprintf("GDT: Loading TSS...\n");
    tss_load(GDT_TSS_SELECTOR);
    
    kprintf("GDT: Initialization complete.\n");
}