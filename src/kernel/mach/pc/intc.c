/*
 * Copyright (c) 2011 Matthew Iselin, Rich Edelman
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
#include <interrupts.h>
#include <apic.h>
#include <pic.h>
#include <io.h>

static int apic_enable = 0;

void init_intc() {
    dprintf("pc: initialising interrupt controller(s)\n");
    if(init_apic() == 0) {
        dprintf("pc: using I/O APIC\n");
        apic_enable = 1;
    } else {
        dprintf("pc: falling back to 8259 PIC\n");
        init_pic();
    }
}

void mach_interrupts_reg(int n, int leveltrig, inthandler_t handler, void *p) {
    if(apic_enable) {
        apic_interrupt_reg(n, leveltrig, handler, p);
    } else {
        pic_interrupt_reg(n, leveltrig, handler, p);
    }
}
