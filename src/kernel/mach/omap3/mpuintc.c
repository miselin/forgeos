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
#include <vmem.h>
#include <util.h>
#include <io.h>
#include <panic.h>
#include <interrupts.h>
#include <mmiopool.h>

#define VECTOR_TABLE_PHYS   0xFFFF0000

#define MPU_INTC_PHYS       0x48200000

static volatile uint32_t    *mpuintc = 0;

#define CONTROL_EXECVECTOR  (1 << 13)

#define MPUINT_REVISION         (0x00) // R
#define MPUINT_SYSCONFIG        (0x10 / 4) // RW
#define MPUINT_SYSSTATUS        (0x14 / 4) // R
#define MPUINT_SIR_IRQ          (0x40 / 4) // R
#define MPUINT_SIR_FIQ          (0x44 / 4) // R
#define MPUINT_CONTROL          (0x48 / 4) // RW
#define MPUINT_PROTECTION       (0x4C / 4) // RW
#define MPUINT_IDLE             (0x50 / 4) // RW
#define MPUINT_IRQ_PRIORITY     (0x60 / 4) // RW
#define MPUINT_FIQ_PRIORITY     (0x64 / 4) // RW
#define MPUINT_THRESHOLD        (0x68 / 4) // RW
#define MPUINT_ITR              (0x80 / 4) // R, multiple entries
#define MPUINT_MIR              (0x84 / 4) // RW, as above
#define MPUINT_MIR_CLEAR        (0x88 / 4) // W, as above
#define MPUINT_MIR_SET          (0x8C / 4) // W, as above
#define MPUINT_ISR_SET          (0x90 / 4) // RW, as above
#define MPUINT_ISR_CLEAR        (0x94 / 4) // W, as above
#define MPUINT_PENDING_IRQ      (0x98 / 4) // R, as above
#define MPUINT_PENDING_FIQ      (0x9C / 4) // R, as above
#define MPUINT_ILR              (0x100 / 4) // RW, multiple entries

const char *g_ExceptionNames[32] = {
    "Interrupt",
    "TLB modification exception",
    "TLB exception (load or instruction fetch)",
    "TLB exception (store)",
    "Address error exception (load or instruction fetch)",
    "Address error exception (store)",
    "Bus error exception (instruction fetch)",
    "Bus error exception (data: load or store)",
    "Syscall exception",
    "Breakpoint exception",
    "Reserved instruction exception",
    "Coprocessor unusable exception",
    "Arithmetic overflow exception",
    "Trap exception",
    "LDCz/SDCz to uncached address",
    "Virtual coherency exception",
    "Machine check exception",
    "Floating point exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Watchpoint exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
};

struct intinfo {
    inthandler_t handler;
    int leveltrig;
    void *param;
};

static struct intinfo interrupts[96];

extern int __arm_vector_table;

void arch_interrupts_init() {
    unative_t vecstart = (unative_t) &__arm_vector_table;
    size_t i = 0;

    // Map in the location for the vector table.
    vmem_map(VECTOR_TABLE_PHYS, vecstart, VMEM_READONLY | VMEM_SUPERVISOR | VMEM_GLOBAL | VMEM_EXEC);

    // Now, map in the MPU interrupt controller for MMIO
    mpuintc = (volatile uint32_t *) mmiopool_alloc(PAGE_SIZE, MPU_INTC_PHYS);

    // Display information.
    uint8_t rev = mpuintc[MPUINT_REVISION] & 0xFF;
    dprintf("ARMv7 MPU Interrupt Controller Revision %d.%d\n", (rev >> 4), (rev & 0xF));

    // Use the high vector for the exception base (0xFFFF0000 instead of 0x0)
    uint32_t sctlr = 0;
    __asm__ volatile("MRC p15, 0, %0, c1, c0, 0" : "=r" (sctlr));
    sctlr |= CONTROL_EXECVECTOR;
    __asm__ volatile("MCR p15, 0, %0, c1, c0, 0" :: "r" (sctlr));

    // Reset the controller
    mpuintc[MPUINT_SYSCONFIG] = 2;
    while((mpuintc[MPUINT_SYSSTATUS] & 1) == 0);

    // Configure auto idle and auto gating.
    mpuintc[MPUINT_IDLE] = 0;

    // Set all interrupts to the highest priority and route all as an IRQ.
    for(i = 0; i < 96; i++) {
        mpuintc[MPUINT_ILR + i] = 0;
    }

    // Mask all interrupts. They will be unmasked in arch_interrupts_reg.
    for(i = 0; i < 3; i++) {
        mpuintc[MPUINT_MIR_SET + (i * 8)] = 0xFFFFFFFF;
        mpuintc[MPUINT_ISR_CLEAR + (i * 8)] = 0xFFFFFFFF;
    }

    // Disable the priority threshold.
    mpuintc[MPUINT_THRESHOLD] = 0xFF;

    // Reset IRQ/FIQ output in case any are pending.
    mpuintc[MPUINT_CONTROL] = 0x3;

    __barrier;

    // Done.
    dprintf("ARMv7 MPU Interrupt Controller init done\n");
}

static void unmask(int n) {
    size_t isr = (n % 96) / 32;
    mpuintc[MPUINT_MIR_CLEAR + (isr * 8)] = 1 << (n % 32);
}

static void mask(int n) {
    size_t isr = (n % 96) / 32;
    mpuintc[MPUINT_MIR_SET + (isr * 8)] = 1 << (n % 32);
}

void arch_interrupts_reg(int n, inthandler_t handler) {
    mach_interrupts_reg(n, 1, handler, 0);
}

void mach_interrupts_reg(int n, int leveltrig, inthandler_t handler, void *param) {
    interrupts[n].handler = handler;
    interrupts[n].leveltrig = leveltrig;
    interrupts[n].param = param;

    if(handler == 0)
        mask(n);
    else
        unmask(n);
}

static void handle(struct intr_stack *p) {
    size_t num = mpuintc[MPUINT_SIR_IRQ] & 0x7F;

    if(interrupts[num].handler) {
        if(!interrupts[num].leveltrig) {
            mpuintc[MPUINT_CONTROL] = 1; // ACK.
        }

        interrupts[num].handler(p, interrupts[num].param);

        if(interrupts[num].leveltrig) {
            mpuintc[MPUINT_CONTROL] = 1; // ACK.
        }
    }
}

void __attribute__((interrupt("FIQ"))) arm_fiq_handler()
{
    dprintf("ARMv7 received FIQ\n");
    while(1);
}

void arm_irq_handler(struct intr_stack *s)
{
    handle(s);
}
