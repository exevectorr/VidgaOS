; ──────────────────────────────────────────────────────────────
; IDT assembly stubs for VidgaOS (NASM, elf32)
; ──────────────────────────────────────────────────────────────

section .text

; ── lidt wrapper ──────────────────────────────────────────────
extern idtp
global idt_load
idt_load:
    lidt [idtp]
    ret

; ── ISR stubs (CPU exceptions 0-31) ──────────────────────────
; Some exceptions push an error code; the rest get a dummy 0.

%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    push dword 0          ; dummy error code
    push dword %1         ; interrupt number
    jmp isr_common
%endmacro

%macro ISR_ERRCODE 1
global isr%1
isr%1:
    ; CPU already pushed error code
    push dword %1         ; interrupt number
    jmp isr_common
%endmacro

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_ERRCODE   30
ISR_NOERRCODE 31

; ── Common ISR handler ────────────────────────────────────────
extern isr_handler
isr_common:
    pusha                 ; push eax,ecx,edx,ebx,esp,ebp,esi,edi
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10          ; kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp              ; pointer to struct regs
    call isr_handler
    add esp, 4

    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8            ; pop int_no + err_code
    iretd

; ── IRQ stubs (hardware interrupts 0-15 → IDT 32-47) ────────
%macro IRQ 2
global irq%1
irq%1:
    push dword 0          ; dummy error code
    push dword %2         ; IDT entry number (32+irq)
    jmp irq_common
%endmacro

IRQ  0, 32
IRQ  1, 33
IRQ  2, 34
IRQ  3, 35
IRQ  4, 36
IRQ  5, 37
IRQ  6, 38
IRQ  7, 39
IRQ  8, 40
IRQ  9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

; ── Common IRQ handler ────────────────────────────────────────
extern irq_handler
irq_common:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call irq_handler
    add esp, 4

    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8
    iretd
