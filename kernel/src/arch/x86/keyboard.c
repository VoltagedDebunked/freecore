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
#include <arch/x86/include/keyboard.h>
#include <arch/x86/include/idt.h>
#include <kernel/io.h>
#include <lib/minstd.h>

/* Keyboard IRQ number */
#define KEYBOARD_IRQ 1
#define KEYBOARD_INT_VECTOR (KEYBOARD_IRQ + 0x20) /* IRQ base is at 0x20 */

/* Keyboard state */
static uint8_t keyboard_state = 0;
static uint8_t keyboard_leds = 0;

/* Circular buffer for keyboard input */
#define KB_BUFFER_SIZE 32
static struct {
    uint8_t data[KB_BUFFER_SIZE];
    size_t head;
    size_t tail;
    size_t count;
} kb_buffer = {0};

/* IO port functions */
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
    while ((inb(PS2_STATUS_PORT) & PS2_STATUS_INPUT_FULL) && timeout--) {
        /* Small delay */
        for (volatile int i = 0; i < 1000; i++);
    }
}

/* Wait for the PS/2 controller to have output ready */
static void ps2_wait_for_output(void) {
    int timeout = 1000;
    while (!(inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL) && timeout--) {
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

/* Send a command to the keyboard */
static uint8_t kb_send_command(uint8_t command) {
    int retries = 3;
    uint8_t response;
    
    while (retries--) {
        ps2_wait_for_input();
        outb(PS2_DATA_PORT, command);
        
        /* Wait for acknowledgement */
        ps2_wait_for_output();
        response = inb(PS2_DATA_PORT);
        
        if (response == KB_RESP_ACK) {
            return response;
        } else if (response == KB_RESP_RESEND) {
            continue;
        } else {
            break;
        }
    }
    
    return response;
}

/* Send a command to the keyboard with a data byte */
static uint8_t kb_send_command_data(uint8_t command, uint8_t data) {
    int retries = 3;
    uint8_t response;
    
    while (retries--) {
        response = kb_send_command(command);
        if (response != KB_RESP_ACK) {
            continue;
        }
        
        ps2_wait_for_input();
        outb(PS2_DATA_PORT, data);
        
        /* Wait for acknowledgement */
        ps2_wait_for_output();
        response = inb(PS2_DATA_PORT);
        
        if (response == KB_RESP_ACK) {
            return response;
        } else if (response == KB_RESP_RESEND) {
            continue;
        } else {
            break;
        }
    }
    
    return response;
}

/* Add a scancode to the keyboard buffer */
static void kb_buffer_add(uint8_t scancode) {
    if (kb_buffer.count < KB_BUFFER_SIZE) {
        kb_buffer.data[kb_buffer.head] = scancode;
        kb_buffer.head = (kb_buffer.head + 1) % KB_BUFFER_SIZE;
        kb_buffer.count++;
    }
}

/* Get a scancode from the keyboard buffer */
static int kb_buffer_get(void) {
    if (kb_buffer.count == 0) {
        return -1; /* Buffer empty */
    }
    
    uint8_t scancode = kb_buffer.data[kb_buffer.tail];
    kb_buffer.tail = (kb_buffer.tail + 1) % KB_BUFFER_SIZE;
    kb_buffer.count--;
    
    return scancode;
}

/* US keyboard layout - scancode set 1 to ASCII mapping */
static const char scancode_to_ascii_low[] = {
    0,    /* 0x00 - Error or no key pressed */
    0,    /* 0x01 - Escape */
    '1',  /* 0x02 */
    '2',  /* 0x03 */
    '3',  /* 0x04 */
    '4',  /* 0x05 */
    '5',  /* 0x06 */
    '6',  /* 0x07 */
    '7',  /* 0x08 */
    '8',  /* 0x09 */
    '9',  /* 0x0A */
    '0',  /* 0x0B */
    '-',  /* 0x0C */
    '=',  /* 0x0D */
    '\b', /* 0x0E - Backspace */
    '\t', /* 0x0F - Tab */
    'q',  /* 0x10 */
    'w',  /* 0x11 */
    'e',  /* 0x12 */
    'r',  /* 0x13 */
    't',  /* 0x14 */
    'y',  /* 0x15 */
    'u',  /* 0x16 */
    'i',  /* 0x17 */
    'o',  /* 0x18 */
    'p',  /* 0x19 */
    '[',  /* 0x1A */
    ']',  /* 0x1B */
    '\n', /* 0x1C - Enter */
    0,    /* 0x1D - Left Control */
    'a',  /* 0x1E */
    's',  /* 0x1F */
    'd',  /* 0x20 */
    'f',  /* 0x21 */
    'g',  /* 0x22 */
    'h',  /* 0x23 */
    'j',  /* 0x24 */
    'k',  /* 0x25 */
    'l',  /* 0x26 */
    ';',  /* 0x27 */
    '\'', /* 0x28 */
    '`',  /* 0x29 */
    0,    /* 0x2A - Left Shift */
    '\\', /* 0x2B */
    'z',  /* 0x2C */
    'x',  /* 0x2D */
    'c',  /* 0x2E */
    'v',  /* 0x2F */
    'b',  /* 0x30 */
    'n',  /* 0x31 */
    'm',  /* 0x32 */
    ',',  /* 0x33 */
    '.',  /* 0x34 */
    '/',  /* 0x35 */
    0,    /* 0x36 - Right Shift */
    '*',  /* 0x37 - Keypad * */
    0,    /* 0x38 - Left Alt */
    ' ',  /* 0x39 - Space */
    0,    /* 0x3A - Caps Lock */
    0,    /* 0x3B - F1 */
    0,    /* 0x3C - F2 */
    0,    /* 0x3D - F3 */
    0,    /* 0x3E - F4 */
    0,    /* 0x3F - F5 */
    0,    /* 0x40 - F6 */
    0,    /* 0x41 - F7 */
    0,    /* 0x42 - F8 */
    0,    /* 0x43 - F9 */
    0,    /* 0x44 - F10 */
    0,    /* 0x45 - Num Lock */
    0,    /* 0x46 - Scroll Lock */
    '7',  /* 0x47 - Keypad 7 */
    '8',  /* 0x48 - Keypad 8 */
    '9',  /* 0x49 - Keypad 9 */
    '-',  /* 0x4A - Keypad - */
    '4',  /* 0x4B - Keypad 4 */
    '5',  /* 0x4C - Keypad 5 */
    '6',  /* 0x4D - Keypad 6 */
    '+',  /* 0x4E - Keypad + */
    '1',  /* 0x4F - Keypad 1 */
    '2',  /* 0x50 - Keypad 2 */
    '3',  /* 0x51 - Keypad 3 */
    '0',  /* 0x52 - Keypad 0 */
    '.',  /* 0x53 - Keypad . */
    0,    /* 0x54 - Alt-SysRq */
    0,    /* 0x55 - F11 or F12? */
    0,    /* 0x56 - International key */
    0,    /* 0x57 - F11 */
    0,    /* 0x58 - F12 */
};

/* US keyboard layout - Shift key mapping */
static const char scancode_to_ascii_high[] = {
    0,    /* 0x00 - Error or no key pressed */
    0,    /* 0x01 - Escape */
    '!',  /* 0x02 */
    '@',  /* 0x03 */
    '#',  /* 0x04 */
    '$',  /* 0x05 */
    '%',  /* 0x06 */
    '^',  /* 0x07 */
    '&',  /* 0x08 */
    '*',  /* 0x09 */
    '(',  /* 0x0A */
    ')',  /* 0x0B */
    '_',  /* 0x0C */
    '+',  /* 0x0D */
    '\b', /* 0x0E - Backspace */
    '\t', /* 0x0F - Tab */
    'Q',  /* 0x10 */
    'W',  /* 0x11 */
    'E',  /* 0x12 */
    'R',  /* 0x13 */
    'T',  /* 0x14 */
    'Y',  /* 0x15 */
    'U',  /* 0x16 */
    'I',  /* 0x17 */
    'O',  /* 0x18 */
    'P',  /* 0x19 */
    '{',  /* 0x1A */
    '}',  /* 0x1B */
    '\n', /* 0x1C - Enter */
    0,    /* 0x1D - Left Control */
    'A',  /* 0x1E */
    'S',  /* 0x1F */
    'D',  /* 0x20 */
    'F',  /* 0x21 */
    'G',  /* 0x22 */
    'H',  /* 0x23 */
    'J',  /* 0x24 */
    'K',  /* 0x25 */
    'L',  /* 0x26 */
    ':',  /* 0x27 */
    '"',  /* 0x28 */
    '~',  /* 0x29 */
    0,    /* 0x2A - Left Shift */
    '|',  /* 0x2B */
    'Z',  /* 0x2C */
    'X',  /* 0x2D */
    'C',  /* 0x2E */
    'V',  /* 0x2F */
    'B',  /* 0x30 */
    'N',  /* 0x31 */
    'M',  /* 0x32 */
    '<',  /* 0x33 */
    '>',  /* 0x34 */
    '?',  /* 0x35 */
    0,    /* 0x36 - Right Shift */
    '*',  /* 0x37 - Keypad * */
    0,    /* 0x38 - Left Alt */
    ' ',  /* 0x39 - Space */
    0,    /* 0x3A - Caps Lock */
    0,    /* 0x3B - F1 */
    0,    /* 0x3C - F2 */
    0,    /* 0x3D - F3 */
    0,    /* 0x3E - F4 */
    0,    /* 0x3F - F5 */
    0,    /* 0x40 - F6 */
    0,    /* 0x41 - F7 */
    0,    /* 0x42 - F8 */
    0,    /* 0x43 - F9 */
    0,    /* 0x44 - F10 */
    0,    /* 0x45 - Num Lock */
    0,    /* 0x46 - Scroll Lock */
    '7',  /* 0x47 - Keypad 7 */
    '8',  /* 0x48 - Keypad 8 */
    '9',  /* 0x49 - Keypad 9 */
    '-',  /* 0x4A - Keypad - */
    '4',  /* 0x4B - Keypad 4 */
    '5',  /* 0x4C - Keypad 5 */
    '6',  /* 0x4D - Keypad 6 */
    '+',  /* 0x4E - Keypad + */
    '1',  /* 0x4F - Keypad 1 */
    '2',  /* 0x50 - Keypad 2 */
    '3',  /* 0x51 - Keypad 3 */
    '0',  /* 0x52 - Keypad 0 */
    '.',  /* 0x53 - Keypad . */
    0,    /* 0x54 - Alt-SysRq */
    0,    /* 0x55 - F11 or F12? */
    0,    /* 0x56 - International key */
    0,    /* 0x57 - F11 */
    0,    /* 0x58 - F12 */
};

/* Convert scancode to ASCII character */
char ps2_scancode_to_ascii(uint8_t scancode, bool release) {
    uint8_t key_code = scancode & 0x7F; /* Remove the release bit */
    
    /* Ignore key releases for ASCII conversion */
    if (release) {
        return 0;
    }
    
    /* Check if key is in range */
    if (key_code >= sizeof(scancode_to_ascii_low) / sizeof(scancode_to_ascii_low[0])) {
        return 0;
    }
    
    /* Handle special keys and update keyboard state */
    switch (key_code) {
        case KB_KEY_LEFT_SHIFT:
        case KB_KEY_RIGHT_SHIFT:
            keyboard_state |= KB_STATE_SHIFT;
            return 0;
            
        case KB_KEY_LEFT_CTRL:
            keyboard_state |= KB_STATE_CTRL;
            return 0;
            
        case KB_KEY_LEFT_ALT:
            keyboard_state |= KB_STATE_ALT;
            return 0;
            
        case KB_KEY_CAPS_LOCK:
            keyboard_state ^= KB_STATE_CAPS_LOCK;
            ps2_keyboard_set_leds(keyboard_leds ^ KB_LED_CAPS_LOCK);
            return 0;
            
        case KB_KEY_NUM_LOCK:
            keyboard_state ^= KB_STATE_NUM_LOCK;
            ps2_keyboard_set_leds(keyboard_leds ^ KB_LED_NUM_LOCK);
            return 0;
            
        case KB_KEY_SCROLL_LOCK:
            keyboard_state ^= KB_STATE_SCROLL_LOCK;
            ps2_keyboard_set_leds(keyboard_leds ^ KB_LED_SCROLL_LOCK);
            return 0;
    }
    
    /* Determine if we need uppercase or lowercase based on state */
    bool uppercase = ((keyboard_state & KB_STATE_SHIFT) != 0) ^ 
                     ((keyboard_state & KB_STATE_CAPS_LOCK) != 0);
    
    /* Return ASCII character */
    if (uppercase) {
        return scancode_to_ascii_high[key_code];
    } else {
        return scancode_to_ascii_low[key_code];
    }
}

/* Keyboard interrupt handler */
void ps2_keyboard_interrupt(void) {
    /* Read scancode from keyboard data port */
    uint8_t scancode = inb(PS2_DATA_PORT);
    
    /* Check for extended keys (E0 prefix) */
    static bool extended = false;
    
    if (scancode == 0xE0) {
        extended = true;
        return;
    }
    
    /* Check if key is pressed or released */
    bool release = (scancode & 0x80) != 0;
    uint8_t key_code = scancode & 0x7F; /* Remove the release bit */
    
    /* Handle key release */
    if (release) {
        switch (key_code) {
            case KB_KEY_LEFT_SHIFT:
            case KB_KEY_RIGHT_SHIFT:
                keyboard_state &= ~KB_STATE_SHIFT;
                break;
                
            case KB_KEY_LEFT_CTRL:
                keyboard_state &= ~KB_STATE_CTRL;
                break;
                
            case KB_KEY_LEFT_ALT:
                keyboard_state &= ~KB_STATE_ALT;
                break;
        }
    }
    
    /* Add scancode to buffer */
    kb_buffer_add(scancode);
    
    /* Reset extended flag */
    extended = false;
}

/* C wrapper for the keyboard interrupt */
static void keyboard_handler(void) {
    ps2_keyboard_interrupt();
    
    /* Send End-of-Interrupt to PIC */
    outb(0x20, 0x20);
}

/* Register the keyboard handler with the IDT */
void ps2_keyboard_register_handler(void) {
    idt_register_handler(KEYBOARD_INT_VECTOR, keyboard_handler);
}

/* Initialize the PS/2 keyboard */
int ps2_keyboard_init(void) {
    kprintf("PS/2 Keyboard: Initializing...\n");
    
    /* Disable both PS/2 ports */
    ps2_send_command(PS2_CMD_DISABLE_PORT1);
    ps2_send_command(PS2_CMD_DISABLE_PORT2);
    
    /* Flush output buffer */
    while (inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL) {
        inb(PS2_DATA_PORT);
    }
    
    /* Read the current configuration */
    ps2_send_command(PS2_CMD_READ_CONFIG);
    ps2_wait_for_output();
    uint8_t config = inb(PS2_DATA_PORT);
    
    /* Modify configuration: enable first port interrupt, disable second port */
    config |= PS2_CONFIG_PORT1_INT;
    config &= ~PS2_CONFIG_PORT2_INT;
    ps2_send_command_data(PS2_CMD_WRITE_CONFIG, config);
    
    /* Perform self-test */
    ps2_send_command(PS2_CMD_SELF_TEST);
    ps2_wait_for_output();
    uint8_t test_result = inb(PS2_DATA_PORT);
    
    if (test_result != 0x55) {
        kerr("PS/2 Keyboard: Controller self-test failed: 0x%x\n", test_result);
        return -1;
    }
    
    /* Test first PS/2 port */
    ps2_send_command(PS2_CMD_TEST_PORT1);
    ps2_wait_for_output();
    test_result = inb(PS2_DATA_PORT);
    
    if (test_result != 0x00) {
        kerr("PS/2 Keyboard: Port 1 test failed: 0x%x\n", test_result);
        return -1;
    }
    
    /* Enable first PS/2 port */
    ps2_send_command(PS2_CMD_ENABLE_PORT1);
    
    /* Reset the keyboard */
    if (kb_send_command(KB_CMD_RESET) != KB_RESP_ACK) {
        kerr("PS/2 Keyboard: Reset command failed\n");
        return -1;
    }
    
    /* Wait for reset response */
    ps2_wait_for_output();
    uint8_t reset_response = inb(PS2_DATA_PORT);
    
    if (reset_response != KB_RESP_SELF_TEST_PASS) {
        kerr("PS/2 Keyboard: Reset failed: 0x%x\n", reset_response);
        return -1;
    }
    
    /* Set default parameters */
    if (kb_send_command(KB_CMD_SET_DEFAULTS) != KB_RESP_ACK) {
        kerr("PS/2 Keyboard: Set defaults command failed\n");
        return -1;
    }
    
    /* Enable scanning */
    if (kb_send_command(KB_CMD_ENABLE_SCANNING) != KB_RESP_ACK) {
        kerr("PS/2 Keyboard: Enable scanning command failed\n");
        return -1;
    }
    
    /* Initialize keyboard buffer */
    kb_buffer.head = 0;
    kb_buffer.tail = 0;
    kb_buffer.count = 0;
    
    /* Set the LEDs based on initial state */
    keyboard_leds = 0;
    ps2_keyboard_set_leds(keyboard_leds);
    
    /* Register the keyboard interrupt handler */
    ps2_keyboard_register_handler();
    
    kprintf("PS/2 Keyboard: Initialization complete\n");
    return 0;
}

/* Set keyboard LEDs */
void ps2_keyboard_set_leds(uint8_t leds) {
    keyboard_leds = leds;
    kb_send_command_data(KB_CMD_SET_LEDS, keyboard_leds);
}

/* Check if a key is available in the buffer */
int ps2_keyboard_available(void) {
    return (kb_buffer.count > 0);
}

/* Get a scancode from the keyboard buffer */
uint8_t ps2_keyboard_get_scancode(void) {
    int scancode = kb_buffer_get();
    if (scancode < 0) {
        return 0; /* No scancode available */
    }
    return (uint8_t)scancode;
}

/* Get a character from the keyboard (waits for input) */
char ps2_keyboard_get_char(void) {
    while (!ps2_keyboard_available()) {
        /* Wait for a key */
    }
    
    uint8_t scancode = ps2_keyboard_get_scancode();
    bool release = (scancode & 0x80) != 0;
    
    char c = ps2_scancode_to_ascii(scancode, release);
    
    /* Keep getting keys until we have a valid character */
    while (c == 0 && ps2_keyboard_available()) {
        scancode = ps2_keyboard_get_scancode();
        release = (scancode & 0x80) != 0;
        c = ps2_scancode_to_ascii(scancode, release);
    }
    
    return c;
}