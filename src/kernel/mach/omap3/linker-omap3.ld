OUTPUT_ARCH(arm)
ENTRY(_init)
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

        . = ALIGN(4);

        __begin_timer_table = .;
        *(.table.timers*);
        __end_timer_table = .;

        . = ALIGN(4096);
        *(.ivt*);
        __end_arm_vector_table = .;
    }

    .bss : {
        . = ALIGN(4096);
        __start_bss = .;
        *(.bss*);
        __end_bss = .;
    }

    . = ALIGN(4096);
    end = .; _end = .; __end = .;
}
