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
#include <stack.h>
#include <util.h>
#include <io.h>

extern int interrupt_handlers;

// 10 bytes: cli, pushl/nop nop, pushl, jmp (no 8-bit displacement)
#define INTERRUPT_STUB_LENGTH		10UL

static inthandler_t interrupts[256];

static struct idt_entry {
	uint16_t base_low;
	uint16_t selector;
	uint8_t always0;
	uint8_t flags;
	uint16_t base_high;
} __packed __aligned(4) idt[256];

static struct idt_ptr {
	uint16_t limit;
	uint32_t base;
} __packed __aligned(4) idtr;

const char* trapnames[] =
{
	"Divide Error",
	"Debug",
	"NMI Interrupt",
	"Breakpoint",
	"Overflow",
	"BOUND Range Exceeded",
	"Invalid Opcode",
	"Device Not Available",
	"Double Fault",
	"Coprocessor Segment Overrun",
	"Invalid TSS",
	"Segment Not Present",
	"Stack Fault",
	"General Protection Fault",
	"Page Fault",
	"Reserved: Interrupt 15",
	"FPU Floating-Point Error",
	"Alignment Check",
	"Machine-Check",
	"SIMD Floating-Point Exception",
	"Reserved: Interrupt 20",
	"Reserved: Interrupt 21",
	"Reserved: Interrupt 22",
	"Reserved: Interrupt 23",
	"Reserved: Interrupt 24",
	"Reserved: Interrupt 25",
	"Reserved: Interrupt 26",
	"Reserved: Interrupt 27",
	"Reserved: Interrupt 28",
	"Reserved: Interrupt 29",
	"Reserved: Interrupt 30",
	"Reserved: Interrupt 31"
};

int cpu_trap(struct intr_stack *stack) {
	uint32_t n = stack->intnum; /// \todo read from stack
	if(interrupts[n] != 0) {
		return interrupts[n](stack);
	} else {
		kprintf("CPU trap #%d", n);
		if(n < 32) {
			uint32_t crn = 0;
			kprintf(": %s\n", trapnames[n]);
			kprintf("EAX: 0x%8x EBX: 0x%8x ECX: 0x%8x EDX: 0x%8x\n", stack->eax, stack->ebx, stack->ecx, stack->edx);
			kprintf("ESI: 0x%8x EDI: 0x%8x ESP: 0x%8x EBP: 0x%8x\n", stack->esi, stack->edi, stack->esp, stack->ebp);
			kprintf("EIP: 0x%8x EFLAGS: 0x%8x\n", stack->eip, stack->eflags);
			__asm__ volatile("mov %%cr0, %0" : "=r" (crn));
			kprintf("CR0: 0x%8x ", crn);
			__asm__ volatile("mov %%cr2, %0" : "=r" (crn));
			kprintf("CR2: 0x%8x ", crn);
			__asm__ volatile("mov %%cr3, %0" : "=r" (crn));
			kprintf("CR3: 0x%8x ", crn);
			__asm__ volatile("mov %%cr4, %0" : "=r" (crn));
			kprintf("CR4: 0x%8x\n", crn);

			kprintf("<system halting>\n");
			while(1) __asm__ volatile("hlt");
		} else
			kprintf(" (unhandled)\n");
	}

	return 0;
}

static void set_idt(size_t n, uintptr_t base, uint16_t sel, uint8_t flags) {
	if(n >= 256)
		return;

	idt[n].base_low = (uint16_t) base & 0xFFFF;
	idt[n].base_high = (uint16_t) (base >> 16) & 0xFFFF;

	idt[n].selector = sel;
	idt[n].always0 = 0;
	idt[n].flags = flags | 0x60;
}

void arch_interrupts_init() {
	size_t i = 0;
	uintptr_t int_stub_base = (uintptr_t) &interrupt_handlers;
	memset(interrupts, 0, sizeof interrupts);

	for(i = 0; i < 256; i++) {
		set_idt(i, int_stub_base + (i * INTERRUPT_STUB_LENGTH), 0x08, 0x8E);
	}

	idtr.limit = (sizeof(struct idt_entry) * 256) - 1;
	idtr.base = (uintptr_t) idt;

	__barrier;

	__asm__ volatile("lidt %0" :: "m" (idtr));
}

void arch_interrupts_enable() {
	__asm__ volatile("sti");
}

void arch_interrupts_disable() {
	__asm__ volatile("cli");
}

void arch_interrupts_reg(int n, inthandler_t handler) {
	interrupts[n] = handler;
}
