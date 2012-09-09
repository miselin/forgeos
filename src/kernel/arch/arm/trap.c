/*
 * Copyright (c) 2012 Matthew Iselin, Rich Edelman
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <types.h>
#include <system.h>
#include <io.h>
#include <panic.h>

void __attribute__((interrupt("SWI"))) arm_swint_handler()
{
    dprintf("ARMv7 received swi trap\n");
}

 void arm_instundef_handler()
{
    panic("undefined instruction");
}

 void arm_reset_handler()
{
    dprintf("ARMv7 received reset trap\n");
}

 void arm_prefetch_abort_handler()
{
    panic("prefetch abort");
}

 void arm_data_abort_handler()
{
    dprintf("ARMv7 data abort\n");

    vaddr_t dfar = 0;
    __asm__ __volatile__("MRC p15,0,%0,c6,c0,0" : "=r" (dfar));

    vaddr_t dfsr = 0;
    __asm__ __volatile__("MRC p15,0,%0,c5,c0,0" : "=r" (dfsr));

    vaddr_t linkreg = 0;
    __asm__ __volatile__("mov %0, lr" : "=r" (linkreg));

    dprintf("at %x status %x lr %x\n", dfar, dfsr, linkreg);

    panic("data abort");
}

 void arm_addrexcept_handler()
{
    panic("address exception");
}

