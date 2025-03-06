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
#include <arch/x86/include/mouse.h>
#include <arch/x86/include/idt.h>
#include <kernel/io.h>
#include <lib/minstd.h>
#include <drivers/driversys.h>

/* Mouse IRQ number */
#define MOUSE_IRQ 12
#define MOUSE_INT_VECTOR (MOUSE_IRQ + 0x20) /* IRQ base is at 0x20 */

/* Driver hooks */
static int ps2_mouse_probe_driver(device_driver_t *driver);
static int ps2_mouse_remove_driver(device_driver_t *driver);

/* Define the PS/2 mouse driver */
static driver_ops_t ps2_mouse_ops = {
    .probe = ps2_mouse_probe_driver,
    .remove = ps2_mouse_remove_driver,
    .suspend = NULL,
    .resume = NULL
};

static device_driver_t ps2_mouse_driver = {
    .name = "ps2_mouse",
    .device_class = DEVICE_CLASS_INPUT,
    .state = DRIVER_STATE_UNLOADED,
    .ops = &ps2_mouse_ops,
    .private_data = NULL
};

/* Mouse state and data */
static mouse_state_t mouse_state = {0};
static mouse_callback_t mouse_callback = NULL;

/* Mouse data packet buffer and state */
static uint8_t mouse_packet_buffer[4];
static uint8_t mouse_packet_index = 0;
static uint8_t mouse_packet_size = 3; /* Default to 3 bytes (standard PS/2 mouse) */

/* IO port functions - duplicated from keyboard.c to avoid dependency */
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Wait for the PS/2 controller to be ready for input */
static void ps2_wait_for_input(void) {
    int timeout = 1000;
    while ((inb(PS2_STATUS_PORT) & 0x02) && timeout--) {
        /* Small delay */
        for (volatile int i = 0; i < 1000; i++);
    }
}

/* Wait for the PS/2 controller to have output ready */
static void ps2_wait_for_output(void) {
    int timeout = 1000;
    while (!(inb(PS2_STATUS_PORT) & 0x01) && timeout--) {
        /* Small delay */
        for (volatile int i = 0; i < 1000; i++);
    }
}

/* Send a command to the PS/2 controller */
static void ps2_send_command(uint8_t command) {
    ps2_wait_for_input();
    outb(PS2_COMMAND_PORT, command);
}

/* Send a command to the PS/2 controller with a data byte */
static void ps2_send_command_data(uint8_t command, uint8_t data) {
    ps2_send_command(command);
    ps2_wait_for_input();
    outb(PS2_DATA_PORT, data);
}

/* Send a command to the mouse */
static uint8_t mouse_send_command(uint8_t command) {
    int retries = 3;
    uint8_t response;

    while (retries--) {
        ps2_wait_for_input();
        outb(PS2_COMMAND_PORT, PS2_CMD_WRITE_MOUSE);
        ps2_wait_for_input();
        outb(PS2_DATA_PORT, command);

        /* Wait for acknowledgement */
        ps2_wait_for_output();
        response = inb(PS2_DATA_PORT);

        if (response == MOUSE_RESP_ACK) {
            return response;
        } else if (response == MOUSE_RESP_NAK) {
            continue;
        } else {
            break;
        }
    }

    return response;
}

/* Send a command to the mouse with a data byte */
static uint8_t mouse_send_command_data(uint8_t command, uint8_t data) {
    int retries = 3;
    uint8_t response;

    while (retries--) {
        response = mouse_send_command(command);
        if (response != MOUSE_RESP_ACK) {
            continue;
        }

        ps2_wait_for_input();
        outb(PS2_COMMAND_PORT, PS2_CMD_WRITE_MOUSE);
        ps2_wait_for_input();
        outb(PS2_DATA_PORT, data);

        /* Wait for acknowledgement */
        ps2_wait_for_output();
        response = inb(PS2_DATA_PORT);

        if (response == MOUSE_RESP_ACK) {
            return response;
        } else if (response == MOUSE_RESP_NAK) {
            continue;
        } else {
            break;
        }
    }

    return response;
}

/* Enable scroll wheel mouse by enabling 4-byte packets */
static void mouse_enable_scroll_wheel(void) {
    /* Magic sequence for enabling scroll wheel:
     * 1. Set sample rate to 200
     * 2. Set sample rate to 100
     * 3. Set sample rate to 80
     * 4. Get device ID (should return 0x03 for scroll wheel mouse)
     */
    ps2_mouse_set_sample_rate(200);
    ps2_mouse_set_sample_rate(100);
    ps2_mouse_set_sample_rate(80);

    mouse_send_command(MOUSE_CMD_GET_DEVICE_ID);
    ps2_wait_for_output();
    uint8_t mouse_id = inb(PS2_DATA_PORT);

    if (mouse_id == MOUSE_RESP_ID_SCROLL) {
        kprintf("PS/2 Mouse: Scroll wheel detected\n");
        mouse_state.has_scroll_wheel = true;
        mouse_packet_size = 4;
    } else {
        kprintf("PS/2 Mouse: Standard mouse detected (ID: 0x%x)\n", mouse_id);
        mouse_state.has_scroll_wheel = false;
        mouse_packet_size = 3;
    }
}

/* Enable 5-button mouse */
static void mouse_enable_5_button(void) {
    /* Magic sequence for enabling 5-button mouse:
     * 1. Set sample rate to 200
     * 2. Set sample rate to 200
     * 3. Set sample rate to 80
     * 4. Get device ID (should return 0x04 for 5-button mouse)
     */
    ps2_mouse_set_sample_rate(200);
    ps2_mouse_set_sample_rate(200);
    ps2_mouse_set_sample_rate(80);

    mouse_send_command(MOUSE_CMD_GET_DEVICE_ID);
    ps2_wait_for_output();
    uint8_t mouse_id = inb(PS2_DATA_PORT);

    if (mouse_id == MOUSE_RESP_ID_5BTN) {
        kprintf("PS/2 Mouse: 5-button mouse detected\n");
        mouse_state.has_5_buttons = true;
        mouse_packet_size = 4;
    }
}

/* Process a complete mouse packet */
static void process_mouse_packet(void) {
    mouse_packet_t packet = {0};

    /* Extract data from packet buffer */
    packet.flags = mouse_packet_buffer[0];
    packet.x_movement = mouse_packet_buffer[1];
    packet.y_movement = mouse_packet_buffer[2];

    if (mouse_packet_size > 3 && mouse_state.has_scroll_wheel) {
        packet.z_movement = (int8_t)(mouse_packet_buffer[3] & MOUSE_PACKET_4_Z_DATA);
        if (mouse_packet_buffer[3] & MOUSE_PACKET_4_Z_SIGN) {
            packet.z_movement = -packet.z_movement;
        }
    }

    /* Update button state */
    mouse_state.buttons = packet.flags & 0x07; /* Get button bits */

    /* Handle movement with overflow checking */
    if (packet.flags & MOUSE_PACKET_X_OVERFLOW) {
        /* X overflow - set to maximum movement in the direction indicated by sign bit */
        if (packet.flags & MOUSE_PACKET_X_SIGN) {
            mouse_state.x -= 128; /* Maximum negative movement */
        } else {
            mouse_state.x += 127; /* Maximum positive movement */
        }
    } else {
        /* Normal X movement */
        mouse_state.x += packet.x_movement;
    }

    if (packet.flags & MOUSE_PACKET_Y_OVERFLOW) {
        /* Y overflow - set to maximum movement in the direction indicated by sign bit */
        if (packet.flags & MOUSE_PACKET_Y_SIGN) {
            mouse_state.y -= 128; /* Maximum negative movement */
        } else {
            mouse_state.y += 127; /* Maximum positive movement */
        }
    } else {
        /* Normal Y movement */
        mouse_state.y -= packet.y_movement; /* Y is inverted in PS/2 protocol */
    }

    /* Update scroll wheel position */
    if (mouse_state.has_scroll_wheel) {
        mouse_state.z += packet.z_movement;
    }

    /* Ensure mouse coordinates stay positive */
    if (mouse_state.x < 0) mouse_state.x = 0;
    if (mouse_state.y < 0) mouse_state.y = 0;

    /* Call registered callback if available */
    if (mouse_callback) {
        mouse_callback(&mouse_state);
    }
}

/* Mouse interrupt handler */
void ps2_mouse_interrupt(void) {
    /* Read data from mouse data port */
    uint8_t data = inb(PS2_DATA_PORT);

    /* Check if this is the start of a new packet */
    if (mouse_packet_index == 0 && !(data & MOUSE_PACKET_ALWAYS_1)) {
        /* Invalid start byte, discard */
        return;
    }

    /* Add byte to packet buffer */
    mouse_packet_buffer[mouse_packet_index] = data;
    mouse_packet_index++;

    /* Check if packet is complete */
    if (mouse_packet_index >= mouse_packet_size) {
        process_mouse_packet();
        mouse_packet_index = 0;
    }
}

/* C wrapper for the mouse interrupt */
static void mouse_handler(void) {
    ps2_mouse_interrupt();

    /* Send End-of-Interrupt to both PICs (since mouse is on the slave PIC) */
    outb(0xA0, 0x20); /* Send EOI to slave PIC */
    outb(0x20, 0x20); /* Send EOI to master PIC */
}

/* Register the mouse handler with the IDT */
void ps2_mouse_register_handler(void) {
    idt_register_handler(MOUSE_INT_VECTOR, mouse_handler);
}

/* Set the mouse sampling rate */
void ps2_mouse_set_sample_rate(uint8_t rate) {
    mouse_send_command_data(MOUSE_CMD_SET_SAMPLE, rate);
    mouse_state.sample_rate = rate;
}

/* Set the mouse resolution */
void ps2_mouse_set_resolution(uint8_t resolution) {
    if (resolution > 3) resolution = 3; /* Clamp to valid values (0-3) */
    mouse_send_command_data(MOUSE_CMD_SET_RES, resolution);
    mouse_state.resolution = resolution;
}

/* Initialize the PS/2 mouse */
int ps2_mouse_init(void) {
    kprintf("PS/2 Mouse: Initializing...\n");

    /* Reset state */
    memset(&mouse_state, 0, sizeof(mouse_state));
    mouse_packet_index = 0;

    /* Enable auxiliary device (mouse) */
    ps2_send_command(0xA8);

    /* Get controller configuration */
    ps2_send_command(0x20);
    ps2_wait_for_output();
    uint8_t status = inb(PS2_DATA_PORT);

    /* Enable interrupts for the auxiliary PS/2 port (mouse) */
    status |= 0x02; /* IRQ12 for mouse */
    ps2_send_command_data(0x60, status);

    /* Reset the mouse */
    if (mouse_send_command(MOUSE_CMD_RESET) != MOUSE_RESP_ACK) {
        kerr("PS/2 Mouse: Reset command failed\n");
        return -1;
    }

    /* Wait for self-test response */
    ps2_wait_for_output();
    uint8_t self_test = inb(PS2_DATA_PORT);
    if (self_test != MOUSE_RESP_SELF_TEST) {
        kerr("PS/2 Mouse: Self-test failed: 0x%x\n", self_test);
        return -1;
    }

    /* Get device ID */
    ps2_wait_for_output();
    uint8_t device_id = inb(PS2_DATA_PORT);
    kprintf("PS/2 Mouse: Device ID: 0x%x\n", device_id);

    /* Try to enable scroll wheel mouse */
    mouse_enable_scroll_wheel();

    /* Try to enable 5-button mouse if we don't have scroll wheel */
    if (!mouse_state.has_scroll_wheel) {
        mouse_enable_5_button();
    }

    /* Set defaults */
    if (mouse_send_command(MOUSE_CMD_DEFAULT) != MOUSE_RESP_ACK) {
        kerr("PS/2 Mouse: Set defaults command failed\n");
        return -1;
    }

    /* Enable mouse data reporting */
    if (mouse_send_command(MOUSE_CMD_ENABLE) != MOUSE_RESP_ACK) {
        kerr("PS/2 Mouse: Enable data reporting failed\n");
        return -1;
    }

    /* Set sampling rate (default to 100 samples/sec) */
    ps2_mouse_set_sample_rate(100);

    /* Set resolution (default to 8 counts/mm) */
    ps2_mouse_set_resolution(2);

    /* Register interrupt handler */
    ps2_mouse_register_handler();

    kprintf("PS/2 Mouse: Initialization complete\n");
    return 0;
}

/* Get the current mouse state */
mouse_state_t* ps2_mouse_get_state(void) {
    return &mouse_state;
}

/* Register a callback function for mouse state changes */
void ps2_mouse_register_callback(mouse_callback_t callback) {
    mouse_callback = callback;
}

/* Probe function for driver registration */
static int ps2_mouse_probe_driver(device_driver_t *driver) {
    return ps2_mouse_init();
}

/* Remove function for driver unregistration */
static int ps2_mouse_remove_driver(device_driver_t *driver) {
    /* Disable mouse data reporting */
    mouse_send_command(MOUSE_CMD_DISABLE);
    return 0;
}

/* Register the PS/2 mouse driver with the driver subsystem */
void ps2_mouse_register_driver(void) {
    device_driver_register(&ps2_mouse_driver);
}

/* Simple mouse state printer for debugging */
void ps2_mouse_debug_callback(mouse_state_t *state) {
    static int print_counter = 0;

    /* Only print every 10th update to avoid flooding the console */
    if (++print_counter >= 10) {
        kprintf("Mouse: X=%d, Y=%d, Z=%d, Buttons=%02x\n",
                state->x, state->y, state->z, state->buttons);
        print_counter = 0;
    }
}