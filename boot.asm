[BITS 32]
section .multiboot
align 4
multiboot_header:
    dd 0x1BADB002
    dd 0x00000003
    dd -(0x1BADB002 + 0x00000003)

section .text
global _start
extern kernel_main

_start:
    mov esp, stack_top
    mov [multiboot_info], ebx
    call kernel_main
    cli
.hang:
    hlt
    jmp .hang

global enable_interrupts, disable_interrupts
enable_interrupts:
    sti
    ret

disable_interrupts:
    cli
    ret

global outb, inb
outb:
    mov edx, [esp + 4]
    mov al, [esp + 8]
    out dx, al
    ret

inb:
    mov edx, [esp + 4]
    in al, dx
    ret

global switch_context
switch_context:
    mov eax, [esp + 4]
    mov esp, eax
    popa
    iret

section .bss
align 16
multiboot_info:
    resd 1
stack_bottom:
    resb 16384
stack_top:
