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

; void gdt_load(struct gdt_ptr *gdt_ptr_addr);
global gdt_load
gdt_load:
    ; Save registers
    push rbp
    mov rbp, rsp
    
    ; Load GDT
    lgdt [rdi]
    
    ; Reload code segment with a far return
    push 0x08            ; Kernel code segment selector (8 = 1 << 3)
    lea rax, [rel .reload_cs]
    push rax
    retfq               ; Far return to reload CS register
.reload_cs:
    ; Reload data segment registers
    mov ax, 0x10        ; Kernel data segment selector (16 = 2 << 3)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Restore registers and return
    pop rbp
    ret

; void tss_load(uint16_t selector);
global tss_load
tss_load:
    ; Save registers
    push rbp
    mov rbp, rsp
    
    ; Load TSS
    mov ax, di           ; First parameter is in rdi (lower 16 bits in di)
    ltr ax
    
    ; Restore registers and return
    pop rbp
    ret