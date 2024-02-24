OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(entry_from_boot)

SECTIONS {
    . = 0x40000;

    /* Text segment: instructions and read-only globals */
    .text : {
        *(.text.unlikely .text.*_unlikely .text.unlikely.*)
        *(.text.exit .text.exit.*)
        *(.text.startup .text.startup.*)
        *(.text.hot .text.hot.*)
        *(.text .stub .text.* .gnu.linkonce.t.*)
    }
    PROVIDE(etext = .);    /* Define the `etext` symbol at this location */
    .rodata : {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
        *(.data.rel.ro.local* .gnu.linkonce.d.rel.ro.local.*)
        *(.data.rel.ro .data.rel.ro.* .gnu.linkonce.d.rel.ro.*)
    }

    /* Data segment: read/write and zero-initialized globals */
    . = ALIGN(4096);       /* Align to a page boundary */
    PROVIDE(start_data = .);
    .data : {
        *(.data .data.* .gnu.linkonce.d.*)
        SORT(CONSTRUCTORS)
    }
    PROVIDE(edata = .);
    .bss : {
        *(.bss .bss.* .gnu.linkonce.b.*)
    }
    PROVIDE(end = .);

    /DISCARD/ : { *(.eh_frame .note.GNU-stack) }
}
