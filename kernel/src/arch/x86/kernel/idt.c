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
#include "../include/asm/idt.h"
#include "../include/asm/gdt.h"
#include "../../../kernel/io.h"
#include "../../../minstd.h"

/* IDT storage */
static struct idt_entry idt[IDT_VECTOR_COUNT];

/* IDT pointer */
static struct idt_ptr idt_ptr;

/* Exception handler table */
static exception_handler exception_handlers[IDT_VECTOR_COUNT] = {0};

/* External interrupt handler stubs defined in idt_asm.asm */
extern void* interrupt_stubs[];

/* Set an IDT entry */
static void idt_set_entry(uint8_t vector, void* handler, uint8_t flags) {
    struct idt_entry* entry = &idt[vector];
    uint64_t addr = (uint64_t)handler;
    
    entry->offset_low = addr & 0xFFFF;
    entry->selector = GDT_KERNEL_CODE_SELECTOR;
    entry->ist = 0;  /* Not using Interrupt Stack Table for now */
    entry->flags = flags;
    entry->offset_mid = (addr >> 16) & 0xFFFF;
    entry->offset_high = (addr >> 32) & 0xFFFFFFFF;
    entry->reserved = 0;
}

/* Halt and catch fire function */
static void idt_hcf(void) {
    kprintf("\nSystem halted.\n");
    for (;;) {
        asm ("hlt");
    }
}

/* Default exception handler */
static void default_exception_handler(void) {
    kerr("Unhandled exception occurred!\n");
    idt_hcf(); /* Halt and catch fire */
}

/* Register an exception handler */
void idt_register_handler(uint8_t vector, exception_handler handler) {
    if (vector < IDT_VECTOR_COUNT) {
        exception_handlers[vector] = handler ? handler : default_exception_handler;
    }
}

/* Initialize the IDT */
void idt_init(void) {
    kprintf("\nIDT: Initializing Interrupt Descriptor Table...\n");
    
    /* Clear the IDT */
    memset(idt, 0, sizeof(idt));
    
    /* Set up IDT pointer */
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint64_t)idt;
    
    /* Set exception handlers */
    for (int i = 0; i < 32; i++) {
        idt_set_entry(i, interrupt_stubs[i], 
                      IDT_FLAGS_PRESENT | IDT_FLAGS_INTERRUPT_GATE | IDT_FLAGS_RING0);
        idt_register_handler(i, NULL); /* Use default handler */
    }
    
    /* Load the IDT */
    idt_load(&idt_ptr);
    
    kprintf("IDT: Initialization complete.\n");
}

/* Error handler for exceptions */
void exception_handler_wrapper(uint64_t vector, uint64_t error_code) {
    kprintf("Exception %llu occurred! Error code: %llu\n", vector, error_code);
    
    /* Call registered handler or default handler */
    if (exception_handlers[vector]) {
        exception_handlers[vector]();
    } else {
        default_exception_handler();
    }
}