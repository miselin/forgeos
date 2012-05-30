/*
 * FORGE Operating System Kernel Linker Script.
 * Kernel starts at 0xC0000000.
 */

OUTPUT_FORMAT(elf32-i386)

ENTRY(_kmain)
SECTIONS {
	. = 0xC0000000;
	init = .; _init = .; __init = .;

	.init : AT(ADDR(.init) - 0xBFF00000) {
	    . = ALIGN(4096);
    	*(.init*);
    	init_end = .; _init_end = .; __init_end = .;
    }

    .text : AT(ADDR(.text) - 0xBFF00000) {
    	. = ALIGN(4096);
    	code = .; _code = .; __code = .;
    	*(.text*);

    	__begin_tests_0 = .;
    	*(.__test.0*);
    	__begin_tests_1 = .;
    	*(.__test.1*);
    	__begin_tests_2 = .;
    	*(.__test.2*);
    	__end_tests = .;
    }

    .data : AT(ADDR(.data) - 0xBFF00000) {
    	. = ALIGN(4096);
    	*(.data*);
    	*(.rodata*);

    	__begin_timer_table = .;
    	*(.table.timers*);
    	__end_timer_table = .;
    }

    .bss : AT(ADDR(.bss) - 0xBFF00000) {
    	. = ALIGN(4096);
    	*(.bss*);
    }

    . = ALIGN(4096);
    end = .; _end = .; __end = .;
}
