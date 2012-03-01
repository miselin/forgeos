OUTPUT_ARCH(arm)
ENTRY(start)
SECTIONS
{
    . = 0x80008000;

    .init : {
        . = ALIGN(4096);
        init = .; _init = .; __init = .;
        *(.init_multiboot*);
        *(.init*);
        init_end = .; _init_end = .; __init_end = .;
    }

    .text : {
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

    .data : {
        . = ALIGN(4096);
        *(.data*);
        *(.rodata*);

        __begin_timer_table = .;
        *(.table.timers*);
        __end_timer_table = .;
    }

    .bss : {
        . = ALIGN(4096);
        *(.bss*);
    }

    . = ALIGN(4096);
    end = .; _end = .; __end = .;
}
