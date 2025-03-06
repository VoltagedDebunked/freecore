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

#ifndef _ASM_X86_MOUSE_H
#define _ASM_X86_MOUSE_H

#include <stdint.h>
#include <stdbool.h>
#include <drivers/driversys.h>

/* PS/2 Controller Ports - shared with keyboard driver */
#define PS2_DATA_PORT           0x60
#define PS2_STATUS_PORT         0x64
#define PS2_COMMAND_PORT        0x64

/* Mouse-specific PS/2 Controller Commands */
#define PS2_CMD_WRITE_MOUSE     0xD4    /* Send following byte to mouse */

/* PS/2 Mouse Commands */
#define MOUSE_CMD_RESET         0xFF
#define MOUSE_CMD_RESEND        0xFE
#define MOUSE_CMD_DEFAULT       0xF6
#define MOUSE_CMD_ENABLE        0xF4
#define MOUSE_CMD_DISABLE       0xF5
#define MOUSE_CMD_SET_SAMPLE    0xF3
#define MOUSE_CMD_GET_DEVICE_ID 0xF2
#define MOUSE_CMD_SET_REMOTE    0xF0
#define MOUSE_CMD_SET_WRAP      0xEE
#define MOUSE_CMD_RESET_WRAP    0xEC
#define MOUSE_CMD_READ_DATA     0xEB
#define MOUSE_CMD_SET_STREAM    0xEA
#define MOUSE_CMD_STATUS_REQ    0xE9
#define MOUSE_CMD_SET_RES       0xE8

/* PS/2 Mouse Responses */
#define MOUSE_RESP_ACK          0xFA
#define MOUSE_RESP_NAK          0xFE
#define MOUSE_RESP_ERROR        0xFC
#define MOUSE_RESP_SELF_TEST    0xAA
#define MOUSE_RESP_ID           0x00    /* Standard PS/2 mouse */
#define MOUSE_RESP_ID_SCROLL    0x03    /* Mouse with scroll wheel */
#define MOUSE_RESP_ID_5BTN      0x04    /* 5-button mouse */

/* Mouse packet flags (first byte) */
#define MOUSE_PACKET_Y_OVERFLOW    0x80
#define MOUSE_PACKET_X_OVERFLOW    0x40
#define MOUSE_PACKET_Y_SIGN        0x20
#define MOUSE_PACKET_X_SIGN        0x10
#define MOUSE_PACKET_ALWAYS_1      0x08
#define MOUSE_PACKET_MIDDLE_BTN    0x04
#define MOUSE_PACKET_RIGHT_BTN     0x02
#define MOUSE_PACKET_LEFT_BTN      0x01

/* Mouse packet flags (fourth byte, for scroll wheel) */
#define MOUSE_PACKET_4_ALWAYS_0    0x80
#define MOUSE_PACKET_4_ALWAYS_0_2  0x40
#define MOUSE_PACKET_4_ALWAYS_0_3  0x30
#define MOUSE_PACKET_4_4TH_BTN     0x10
#define MOUSE_PACKET_4_5TH_BTN     0x20
#define MOUSE_PACKET_4_Z_SIGN      0x08
#define MOUSE_PACKET_4_Z_DATA      0x07

/* Scroll directions */
#define MOUSE_SCROLL_UP           1
#define MOUSE_SCROLL_DOWN        -1
#define MOUSE_SCROLL_NONE         0

/* Mouse state structure */
typedef struct {
    int x;                  /* Current X position */
    int y;                  /* Current Y position */
    int z;                  /* Scroll wheel position */
    uint8_t buttons;        /* Button state */
    bool has_scroll_wheel;  /* True if mouse has scroll wheel */
    bool has_5_buttons;     /* True if mouse has 5 buttons */
    int resolution;         /* Current resolution setting (0-3) */
    int sample_rate;        /* Current sample rate (in Hz) */
} mouse_state_t;

/* Mouse packet structure */
typedef struct {
    uint8_t flags;          /* Status flags (first byte) */
    int8_t x_movement;      /* X movement (second byte) */
    int8_t y_movement;      /* Y movement (third byte) */
    int8_t z_movement;      /* Scroll wheel movement (fourth byte if available) */
} mouse_packet_t;

/* Mouse data callback function type */
typedef void (*mouse_callback_t)(mouse_state_t *state);

/* Function prototypes */
int ps2_mouse_init(void);
void ps2_mouse_register_driver(void);
void ps2_mouse_interrupt(void);
void ps2_mouse_register_handler(void);
void ps2_mouse_set_sample_rate(uint8_t rate);
void ps2_mouse_set_resolution(uint8_t resolution);

/* Get the current mouse state */
mouse_state_t* ps2_mouse_get_state(void);

/* Register a callback for mouse state changes */
void ps2_mouse_register_callback(mouse_callback_t callback);

/* Debug callback function that prints mouse state to console */
void ps2_mouse_debug_callback(mouse_state_t *state);

#endif /* _ASM_X86_MOUSE_H */