/* Linker script for VidgaOS - GRUB-compatible kernel */

ENTRY(_start)

SECTIONS {
    /* Kernel starts at 1MB (standard for GRUB) */
    . = 0x100000;
    
    .multiboot : {
        *(.multiboot)
    }
    
    .text : ALIGN(4K) {
        *(.text)
        *(.text.*)
    }
    
    .rodata : ALIGN(4K) {
        *(.rodata)
        *(.rodata.*)
    }
    
    .data : ALIGN(4K) {
        *(.data)
        *(.data.*)
    }
    
    .bss : ALIGN(4K) {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }
    
    /DISCARD/ : {
        *(.note.GNU-stack)
        *(.gnu_debuglink)
        *(.gnu_debugdata)
    }
}
