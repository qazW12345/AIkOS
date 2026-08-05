OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

SECTIONS
{
    . = 0x100000;               /* kernel load address (ADR-006) */

    .text : { *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }

    .bss : {
        *(COMMON)
        *(.bss*)
        . = ALIGN(16);
        stack_bottom = .;
        . = . + 0x4000;         /* 16 KiB kernel stack */
        stack_top = .;
    }
}
