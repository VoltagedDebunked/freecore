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
#include <stdbool.h>
#include <limine.h>
#include "minstd.h"
#include "arch/x86/include/asm/gdt.h"
#include "kernel/io.h"
#include "kernel/config.h"

#ifdef __x86_64__
#include "arch/x86/include/asm/serial.h"
#endif

/* Limine requests */
__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_bootloader_info_request bootloader_info_request = {
    .id = LIMINE_BOOTLOADER_INFO_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

/* Halt and catch fire function */
static void hcf(void) {
    kprintf("\nSystem halted.\n");
    for (;;) {
        asm ("hlt");
    }
}

/* Kernel entry point */
void kmain(void) {
    /* Initialize I/O (includes serial) */
    io_init();
    
    /* Print welcome message */
    kprintf("\n");
    kprintf("FreeCore Kernel - Starting up...\n");
    kprintf("--------------------------------\n");
    
    /* Ensure the bootloader actually understands our base revision (see spec) */
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        kerr("Incompatible Limine bootloader detected!\n");
        hcf();
    }

    /* Print bootloader info if available */
    if (bootloader_info_request.response != NULL) {
        kprintf("Bootloader: %s %s\n", 
                bootloader_info_request.response->name,
                bootloader_info_request.response->version);
    }

    /* Initialize the GDT */
    kprintf("Initializing GDT... ");
    gdt_init();
    kprintf("done\n");

    /* Ensure we got a framebuffer */
    kprintf("Checking framebuffer... ");
    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        kprintf("failed\n");
        kerr("No framebuffer available!\n");
        hcf();
    }
    kprintf("detected\n");

    /* Fetch the first framebuffer */
    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];
    kprintf("Framebuffer: %ux%u, %u BPP\n", 
            (unsigned)framebuffer->width, 
            (unsigned)framebuffer->height,
            (unsigned)framebuffer->bpp);

    /* Clear the screen with dark blue */
    kprintf("Clearing screen... ");
    memset((void *)framebuffer->address, 0, framebuffer->pitch * framebuffer->height);
    kprintf("done\n");
    
    /* Initialization complete */
    kprintf("\nFreeCore v%s initialization complete!\n", KERNEL_VERSION_STRING);
    kprintf("Serial communication is working on COM port %d.\n", 
            DEBUG_SERIAL_PORT == COM1_PORT ? 1 :
            DEBUG_SERIAL_PORT == COM2_PORT ? 2 :
            DEBUG_SERIAL_PORT == COM3_PORT ? 3 :
            DEBUG_SERIAL_PORT == COM4_PORT ? 4 : 0);
    kprintf("Press any key to receive echo: ");
    
    /* Echo received characters (simple terminal) */
    while (1) {
        char c = serial_read_char(DEBUG_SERIAL_PORT);
        serial_write_char(DEBUG_SERIAL_PORT, c);
        
        /* Add line feed after carriage return */
        if (c == '\r') {
            serial_write_char(DEBUG_SERIAL_PORT, '\n');
        }
    }
    
    /* We should never get here */
    hcf();
}