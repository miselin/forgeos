/*
 * Mattise Kernel Linker Script.
 * Kernel starts at 0xC0000000.
 */

OUTPUT_FORMAT(elf32-i386)

ENTRY(_start)
SECTIONS {
	. = 0xC0000000;

	.init : AT(ADDR(.init) - 0xBFF00000) {
	    . = ALIGN(4096);
    	init = .; _init = .; __init = .;
    	*(.init_multiboot*);
    	*(.init*);
    	init_end = .; _init_end = .; __init_end = .;
    }

    .text : AT(ADDR(.text) - 0xBFF00000) {
    	. = ALIGN(4096);
    	code = .; _code = .; __code = .;
    	*(.text*);
    }

    .data : AT(ADDR(.data) - 0xBFF00000) {
    	. = ALIGN(4096);
    	*(.data*);
    	*(.rodata*);
    }

    .bss : AT(ADDR(.bss) - 0xBFF00000) {
    	. = ALIGN(4096);
    	*(.bss*);
    }

    . = ALIGN(4096);
    end = .; _end = .; __end = .;
}
