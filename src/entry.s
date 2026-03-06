MB_MAGIC    equ 0x1BADB002
MB_FLAGS    equ 0
MB_CHECKSUM equ -(MB_MAGIC + MB_FLAGS)

section .multiboot
align 4
    dd MB_MAGIC
    dd MB_FLAGS
    dd MB_CHECKSUM

; ── GDT ──────────────────────────────────────────────────────
section .data
align 16
gdt_start:
    ; Null descriptor (0x00)
    dq 0

    ; Kernel code segment (0x08): base=0, limit=4GB, 32-bit, ring 0
    dw 0xFFFF       ; limit low
    dw 0x0000       ; base low
    db 0x00         ; base mid
    db 10011010b    ; access: present, ring0, code, readable
    db 11001111b    ; flags: 4KB granularity, 32-bit + limit high (0xF)
    db 0x00         ; base high

    ; Kernel data segment (0x10): base=0, limit=4GB, 32-bit, ring 0
    dw 0xFFFF       ; limit low
    dw 0x0000       ; base low
    db 0x00         ; base mid
    db 10010010b    ; access: present, ring0, data, writable
    db 11001111b    ; flags: 4KB granularity, 32-bit + limit high (0xF)
    db 0x00         ; base high
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1   ; size
    dd gdt_start                  ; offset

; ── BSS ──────────────────────────────────────────────────────
section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

; ── Text ─────────────────────────────────────────────────────
section .text
extern kernel_main
global _start

_start:
    cli                 ; disable interrupts until IDT is ready
    mov esp, stack_top

    ; Load our own GDT so selectors 0x08/0x10 are defined
    lgdt [gdt_descriptor]

    ; Reload code segment via far jump
    jmp 0x08:.reload_cs
.reload_cs:
    ; Reload all data segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    push ebx        
    push eax        

    call kernel_main
    
    hlt
    jmp $
