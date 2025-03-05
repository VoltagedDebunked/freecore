; FreeCore - A free operating system kernel
; Copyright (C) 2025 FreeCore Development Team
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.

[BITS 64]
section .text

; External function to handle exceptions
extern exception_handler_wrapper

; IDT Load function (similar to GDT load)
global idt_load
idt_load:
    ; Load IDT
    lidt [rdi]
    ret

; Macro to generate interrupt stub with error code
%macro INTERRUPT_STUB_ERROR_CODE 1
global interrupt_stub_%1
interrupt_stub_%1:
    ; Push the interrupt vector
    push %1
    
    ; Jump to common handler
    jmp interrupt_common_stub
%endmacro

; Macro to generate interrupt stub without error code
%macro INTERRUPT_STUB_NO_ERROR_CODE 1
global interrupt_stub_%1
interrupt_stub_%1:
    ; Push a dummy error code (0)
    push 0
    
    ; Push the interrupt vector
    push %1
    
    ; Jump to common handler
    jmp interrupt_common_stub
%endmacro

; Common stub for all interrupt handlers
global interrupt_common_stub
interrupt_common_stub:
    ; Save all registers
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; Call exception handler
    mov rdi, [rsp + 16*8]    ; Vector number
    mov rsi, [rsp + 15*8]    ; Error code
    call exception_handler_wrapper

    ; Restore registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    ; Remove vector and error code
    add rsp, 16

    ; Return from interrupt
    iretq

; Generate specific interrupt stubs
INTERRUPT_STUB_NO_ERROR_CODE 0     ; Divide by Zero
INTERRUPT_STUB_NO_ERROR_CODE 1     ; Debug
INTERRUPT_STUB_NO_ERROR_CODE 2     ; Non-Maskable Interrupt
INTERRUPT_STUB_NO_ERROR_CODE 3     ; Breakpoint
INTERRUPT_STUB_NO_ERROR_CODE 4     ; Overflow
INTERRUPT_STUB_NO_ERROR_CODE 5     ; Bound Range Exceeded
INTERRUPT_STUB_NO_ERROR_CODE 6     ; Invalid Opcode
INTERRUPT_STUB_NO_ERROR_CODE 7     ; Device Not Available
INTERRUPT_STUB_ERROR_CODE    8     ; Double Fault
INTERRUPT_STUB_NO_ERROR_CODE 9     ; Coprocessor Segment Overrun
INTERRUPT_STUB_ERROR_CODE    10    ; Invalid TSS
INTERRUPT_STUB_ERROR_CODE    11    ; Segment Not Present
INTERRUPT_STUB_ERROR_CODE    12    ; Stack-Segment Fault
INTERRUPT_STUB_ERROR_CODE    13    ; General Protection Fault
INTERRUPT_STUB_ERROR_CODE    14    ; Page Fault
INTERRUPT_STUB_NO_ERROR_CODE 15    ; Reserved
INTERRUPT_STUB_NO_ERROR_CODE 16    ; x87 Floating-Point Exception
INTERRUPT_STUB_ERROR_CODE    17    ; Alignment Check
INTERRUPT_STUB_NO_ERROR_CODE 18    ; Machine Check
INTERRUPT_STUB_NO_ERROR_CODE 19    ; SIMD Floating-Point Exception
INTERRUPT_STUB_NO_ERROR_CODE 20    ; Virtualization Exception
INTERRUPT_STUB_NO_ERROR_CODE 21    ; Control Protection Exception
INTERRUPT_STUB_NO_ERROR_CODE 22    ; Reserved
INTERRUPT_STUB_NO_ERROR_CODE 23    ; Reserved
INTERRUPT_STUB_NO_ERROR_CODE 24    ; Reserved
INTERRUPT_STUB_NO_ERROR_CODE 25    ; Reserved
INTERRUPT_STUB_NO_ERROR_CODE 26    ; Reserved
INTERRUPT_STUB_NO_ERROR_CODE 27    ; Reserved
INTERRUPT_STUB_NO_ERROR_CODE 28    ; Hypervisor Injection Exception
INTERRUPT_STUB_NO_ERROR_CODE 29    ; VMM Communication Exception
INTERRUPT_STUB_ERROR_CODE    30    ; Security Exception
INTERRUPT_STUB_NO_ERROR_CODE 31    ; Reserved

; Create an array of interrupt stub pointers
section .data
global interrupt_stubs
interrupt_stubs:
    dq interrupt_stub_0
    dq interrupt_stub_1
    dq interrupt_stub_2
    dq interrupt_stub_3
    dq interrupt_stub_4
    dq interrupt_stub_5
    dq interrupt_stub_6
    dq interrupt_stub_7
    dq interrupt_stub_8
    dq interrupt_stub_9
    dq interrupt_stub_10
    dq interrupt_stub_11
    dq interrupt_stub_12
    dq interrupt_stub_13
    dq interrupt_stub_14
    dq interrupt_stub_15
    dq interrupt_stub_16
    dq interrupt_stub_17
    dq interrupt_stub_18
    dq interrupt_stub_19
    dq interrupt_stub_20
    dq interrupt_stub_21
    dq interrupt_stub_22
    dq interrupt_stub_23
    dq interrupt_stub_24
    dq interrupt_stub_25
    dq interrupt_stub_26
    dq interrupt_stub_27
    dq interrupt_stub_28
    dq interrupt_stub_29
    dq interrupt_stub_30
    dq interrupt_stub_31