OUTPUT_FORMAT("elf32-powerpc");

/* SYMBOLS_GO_HERE */

SECTIONS {
    /* The second cave (see the Makefile): the walking. Listed first, so that these input
     * sections land here and not in .text below. */
    . = CODE_CAVE2_START;

    .text2 : {
        obj/walk.o(.text .text.* .rodata*)
        obj/walk_hooks.o(.text .text.*)
        obj/item_hooks.o(.text .text.* .rodata*)
    }
    ASSERT(. <= CODE_CAVE2_END, "the walking does not fit in the second cave")

    . = CODE_CAVE_START;

    .text : {
        *(.text)
        *(.text.*)
        *(.gnu.linkonce.t*)
    }

    .rodata ALIGN(8) : { *(.rodata*) }
    ASSERT(. <= CODE_CAVE_END, "the code does not fit in the cave")

    . = GLOBALS_START;

    .data ALIGN(8) : { *(.data*) }
    .sdata ALIGN(8) : { *(.sdata*) }
    ASSERT(SIZEOF(.data) == 0 && SIZEOF(.sdata) == 0, "globals must be zero-initialised (.bss only): the patch never writes them")
    .bss ALIGN(8) : { *(.bss*) *(.sbss*) *(COMMON) }
    ASSERT(. <= GLOBALS_END, "the globals overrun the data in data/")

    /* HOOKS_GO_HERE */

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame)
        *(.gnu.attributes)
    }
}
