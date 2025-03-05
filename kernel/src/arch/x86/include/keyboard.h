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

#ifndef _ASM_X86_KEYBOARD_H
#define _ASM_X86_KEYBOARD_H

#include <stdint.h>
#include <drivers/driversys.h>
#include <stdbool.h>

/* PS/2 Controller Ports */
#define PS2_DATA_PORT           0x60
#define PS2_STATUS_PORT         0x64
#define PS2_COMMAND_PORT        0x64

/* PS/2 Controller Commands */
#define PS2_CMD_READ_CONFIG     0x20
#define PS2_CMD_WRITE_CONFIG    0x60
#define PS2_CMD_DISABLE_PORT2   0xA7
#define PS2_CMD_ENABLE_PORT2    0xA8
#define PS2_CMD_TEST_PORT2      0xA9
#define PS2_CMD_SELF_TEST       0xAA
#define PS2_CMD_TEST_PORT1      0xAB
#define PS2_CMD_DISABLE_PORT1   0xAD
#define PS2_CMD_ENABLE_PORT1    0xAE
#define PS2_CMD_WRITE_PORT2     0xD4

/* PS/2 Controller Status Register Bits */
#define PS2_STATUS_OUTPUT_FULL  0x01
#define PS2_STATUS_INPUT_FULL   0x02
#define PS2_STATUS_SYSTEM_FLAG  0x04
#define PS2_STATUS_COMMAND_DATA 0x08
#define PS2_STATUS_TIMEOUT      0x40
#define PS2_STATUS_PARITY_ERROR 0x80

/* PS/2 Configuration Byte Bits */
#define PS2_CONFIG_PORT1_INT    0x01
#define PS2_CONFIG_PORT2_INT    0x02
#define PS2_CONFIG_SYSTEM_FLAG  0x04
#define PS2_CONFIG_ZERO1        0x08
#define PS2_CONFIG_PORT1_CLOCK  0x10
#define PS2_CONFIG_PORT2_CLOCK  0x20
#define PS2_CONFIG_PORT1_TRANSLATION 0x40
#define PS2_CONFIG_ZERO2        0x80

/* Keyboard Commands */
#define KB_CMD_SET_LEDS         0xED
#define KB_CMD_ECHO             0xEE
#define KB_CMD_GET_SET_SCANCODE 0xF0
#define KB_CMD_IDENTIFY         0xF2
#define KB_CMD_SET_TYPEMATIC    0xF3
#define KB_CMD_ENABLE_SCANNING  0xF4
#define KB_CMD_DISABLE_SCANNING 0xF5
#define KB_CMD_SET_DEFAULTS     0xF6
#define KB_CMD_SET_ALL_TYPEMATIC 0xF7
#define KB_CMD_SET_ALL_MAKE_RELEASE 0xF8
#define KB_CMD_SET_ALL_MAKE     0xF9
#define KB_CMD_SET_ALL_TYPEMATIC_MAKE_RELEASE 0xFA
#define KB_CMD_SET_KEY_TYPEMATIC 0xFB
#define KB_CMD_SET_KEY_MAKE_RELEASE 0xFC
#define KB_CMD_SET_KEY_MAKE     0xFD
#define KB_CMD_RESEND           0xFE
#define KB_CMD_RESET            0xFF

/* Keyboard Responses */
#define KB_RESP_ACK             0xFA
#define KB_RESP_RESEND          0xFE
#define KB_RESP_ERROR           0x00
#define KB_RESP_SELF_TEST_PASS  0xAA
#define KB_RESP_ECHO_RESPONSE   0xEE

/* Special Key Codes */
#define KB_KEY_ESCAPE           0x01
#define KB_KEY_BACKSPACE        0x0E
#define KB_KEY_TAB              0x0F
#define KB_KEY_ENTER            0x1C
#define KB_KEY_LEFT_CTRL        0x1D
#define KB_KEY_LEFT_SHIFT       0x2A
#define KB_KEY_RIGHT_SHIFT      0x36
#define KB_KEY_LEFT_ALT         0x38
#define KB_KEY_SPACE            0x39
#define KB_KEY_CAPS_LOCK        0x3A
#define KB_KEY_F1               0x3B
#define KB_KEY_F2               0x3C
#define KB_KEY_F3               0x3D
#define KB_KEY_F4               0x3E
#define KB_KEY_F5               0x3F
#define KB_KEY_F6               0x40
#define KB_KEY_F7               0x41
#define KB_KEY_F8               0x42
#define KB_KEY_F9               0x43
#define KB_KEY_F10              0x44
#define KB_KEY_NUM_LOCK         0x45
#define KB_KEY_SCROLL_LOCK      0x46
#define KB_KEY_HOME             0x47
#define KB_KEY_UP               0x48
#define KB_KEY_PAGE_UP          0x49
#define KB_KEY_LEFT             0x4B
#define KB_KEY_RIGHT            0x4D
#define KB_KEY_END              0x4F
#define KB_KEY_DOWN             0x50
#define KB_KEY_PAGE_DOWN        0x51
#define KB_KEY_INSERT           0x52
#define KB_KEY_DELETE           0x53
#define KB_KEY_F11              0x57
#define KB_KEY_F12              0x58

/* Extended Keys (E0 prefix) */
#define KB_KEY_RIGHT_ALT        0xE038
#define KB_KEY_RIGHT_CTRL       0xE01D
#define KB_KEY_LEFT_WINDOWS     0xE05B
#define KB_KEY_RIGHT_WINDOWS    0xE05C
#define KB_KEY_MENU             0xE05D

/* Keyboard LED states */
#define KB_LED_SCROLL_LOCK      0x01
#define KB_LED_NUM_LOCK         0x02
#define KB_LED_CAPS_LOCK        0x04

/* Keyboard state flags */
#define KB_STATE_SHIFT          0x01
#define KB_STATE_CTRL           0x02
#define KB_STATE_ALT            0x04
#define KB_STATE_CAPS_LOCK      0x08
#define KB_STATE_NUM_LOCK       0x10
#define KB_STATE_SCROLL_LOCK    0x20
#define KB_STATE_EXTENDED       0x40

/* Function prototypes */
int ps2_keyboard_init(void);
void ps2_keyboard_handler(void);
char ps2_keyboard_get_char(void);
int ps2_keyboard_available(void);
void ps2_keyboard_set_leds(uint8_t leds);
uint8_t ps2_keyboard_get_scancode(void);
char ps2_scancode_to_ascii(uint8_t scancode, bool release);

/* Register the keyboard handler with the IDT */
void ps2_keyboard_register_handler(void);
void ps2_keyboard_register_driver(void);

#endif /* _ASM_X86_KEYBOARD_H */