/*
 * Copyright (c) 2012 Matthew Iselin
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
#include <mmiopool.h>
#include <system.h>
#include <malloc.h>
#include <assert.h>
#include <util.h>
#include <vmem.h>
#include <io.h>

#include <acpi.h>

struct lapic;

static void *ioapic_list = 0;
static struct lapic *lapic = 0;

static void *interrupt_override = 0;

#define IOAPIC_IOREGSEL         0
#define IOAPIC_IOWIN            0x10
#define IOAPIC_MMIOSIZE         0x14

#define IOAPIC_REG_ID           0x00
#define IOAPIC_REG_VER          0x01
#define IOAPIC_REG_ARBID        0x02
#define IOAPIC_REG_REDIRBASE    0x10

#define IOAPIC_MAXREDIR_SHIFT   16
#define IOAPIC_MAXREDIR_MASK    0xFF

#define IOAPIC_ACPIVER_SHIFT    0
#define IOAPIC_ACPIVER_MASK     0xFF

#define IOAPIC_INT_BASE         0x30
#define LAPIC_SPURIOUS          0xFF

#define OVERRIDE_POLARITY_CONFORMS      0
#define OVERRIDE_POLARITY_ACTIVEHIGH    1
#define OVERRIDE_POLARITY_ACTIVELOW     3

#define OVERRIDE_TRIGGER_CONFORMS       0
#define OVERRIDE_TRIGGER_EDGE           1
#define OVERRIDE_TRIGGER_LEVEL          3

struct irqhandler {
    inthandler_t    handler;
    void            *param;
    size_t          actual; // Actual IRQ number (when routing is active).
};

struct override {
    /// Original IRQ (ie, the ISA IRQ number)
    size_t          origirq;

    /// Connected I/O APIC IRQ.
    size_t          newirq;

    /// Polarity of the input signal.
    uint8_t         polarity;

    /// Trigger mode of the input signal.
    uint8_t         trigger;
};

struct lapic {
    uint8_t acpi_id;

    uint8_t lapic_id;

    uint8_t proc_id;

    paddr_t physaddr;
    vaddr_t mmioaddr;
};

struct ioapic {
    uint8_t acpi_id;

    // Nmaed differently to avoid massive confusion and typos.
    uint8_t ioapic_id;

    paddr_t physaddr;
    vaddr_t mmioaddr;

    /// IRQ line at the base of this I/O APIC
    uint32_t irqbase;

    /// Installed ISR handler base for the I/O APIC
    uint32_t intbase;

    /// Number of IRQs on this I/O APIC
    uint32_t intcount;

    /// IRQ handler instances.
    struct irqhandler *handlers;
};

static uint32_t read_lapic_reg(vaddr_t mmio, uint16_t reg) {
    return *((volatile uint32_t *) (mmio + reg));
}

static void write_lapic_reg(vaddr_t mmio, uint16_t reg, uint32_t val) {
    *((volatile uint32_t *) (mmio + reg)) = val;
}

static uint32_t read_ioapic_reg(vaddr_t mmio, uint8_t reg) {
    *((volatile uint32_t *) (mmio + IOAPIC_IOREGSEL)) = reg;
    __barrier;
    return *((volatile uint32_t *) (mmio + IOAPIC_IOWIN));
}

static void write_ioapic_reg(vaddr_t mmio, uint8_t reg, uint32_t val) {
    *((volatile uint32_t *) (mmio + IOAPIC_IOREGSEL)) = reg;
    __barrier;
    *((volatile uint32_t *) (mmio + IOAPIC_IOWIN)) = val;
}

static void lapic_ack() {
    // Send an EOI to the LAPIC
    write_lapic_reg(lapic->mmioaddr, 0xB0, 0);
}

static int handle_ioapic_irq(struct intr_stack *s, void *p __unused) {
    uint32_t intnum = s->intnum - IOAPIC_INT_BASE;

    // Find the I/O APIC for this particular IRQ.
    size_t n = 0;
    struct ioapic *meta = 0;
    while((meta = (struct ioapic *) list_at(ioapic_list, n++))) {
        if((meta->intbase <= s->intnum) && (meta->intcount > intnum)) {
            break;
        }
    }

    if(!meta) {
        lapic_ack();
        return 0;
    }

    // Handle the IRQ.
    int ret = 0;
    if(meta->handlers[intnum].handler) {
        ret = meta->handlers[intnum].handler(s, meta->handlers[intnum].param);
    }

    lapic_ack();

    return ret;
}

static int lapic_localint(struct intr_stack *s, void *p __unused) {
    if(s->intnum == LAPIC_SPURIOUS) {
        dprintf("Local APIC: spurious interrupt\n");
    } else {
        dprintf("Local APIC: local interrupt\n");
        lapic_ack();
    }

    return 0;
}

void init_lapic() {
    // Set the spurious interrupt vector, and enable the APIC.
    uint32_t svr = read_lapic_reg(lapic->mmioaddr, 0xF0);
    svr &= ~(0x1FF);
    svr |= (1 << 8); // Enable APIC.
    svr |= LAPIC_SPURIOUS;
    write_lapic_reg(lapic->mmioaddr, 0xF0, svr);

    interrupts_trap_reg(LAPIC_SPURIOUS, lapic_localint);

    // Save the Local APIC ID.
    lapic->lapic_id = (uint8_t) (read_lapic_reg(lapic->mmioaddr, 0x20) >> 24);

    // Task priority = 0.
    uint32_t taskprio = read_lapic_reg(lapic->mmioaddr, 0x80);
    taskprio &= ~0xFF;
    write_lapic_reg(lapic->mmioaddr, 0x80, taskprio);
}

int init_apic() {
    ACPI_STATUS status;
    ACPI_TABLE_MADT *madt;

    // Don't even bother trying if there's no APIC.
    uint32_t a, b, c, d;
    x86_cpuid(1, &a, &b, &c, &d);
    if((d & (1 << 9)) == 0) {
        dprintf("apic: no apic present according to cpuid\n");
        return -1;
    }

    status = AcpiGetTable((ACPI_STRING) ACPI_SIG_MADT, 0, ACPI_CAST_INDIRECT_PTR(ACPI_TABLE_HEADER, &madt));
    if(ACPI_FAILURE(status)) {
        dprintf("ioapic: couldn't get MADT, ouch\n");
        return -1;
    }
    status = AcpiGetTableHeader((ACPI_STRING) ACPI_SIG_MADT, 0, (ACPI_TABLE_HEADER *) madt);

    // Create the list of I/O APICs, ready for appending to.
    ioapic_list = create_list();
    interrupt_override = create_tree();

    // Enable the LAPIC, if by chance one exists we'll want to use it.
    x86_get_msr(0x1B, &a, &b);
    a |= (1 << 11);
    x86_set_msr(0x1B, a, b);

    // Map in the LAPIC as needed.
    paddr_t lapic_paddr = (a & ~(PAGE_SIZE - 1)) | ((b & 0x3) << 29);
    vmem_map(KERNEL_LAPIC, lapic_paddr, VMEM_SUPERVISOR | VMEM_GLOBAL | VMEM_READWRITE);
    lapic = (struct lapic *) malloc(sizeof(struct lapic));
    lapic->physaddr = lapic_paddr;
    lapic->mmioaddr = KERNEL_LAPIC;

    // Initialise our LAPIC.
    init_lapic();

    // Parse all structures in the table.
    uintptr_t base = ((uintptr_t) madt) + sizeof(*madt);
    size_t sz = madt->Header.Length - sizeof(*madt);
    while(sz) {
        ACPI_SUBTABLE_HEADER *hdr = (ACPI_SUBTABLE_HEADER *) base;

        if(hdr->Type == ACPI_MADT_TYPE_LOCAL_APIC) {
            ACPI_MADT_LOCAL_APIC *lapic_meta = (ACPI_MADT_LOCAL_APIC *) base;
            dprintf("Local APIC ID=%x, ProcessorId=%x, Flags=%x\n", lapic_meta->Id, lapic_meta->ProcessorId, lapic_meta->LapicFlags);

            if(lapic_meta->LapicFlags & ACPI_MADT_ENABLED) {
                dprintf("Processor is usable!\n");

                /// \todo Store information so we can startup APs.

                // Is this a match for the BSP?
                if(lapic_meta->Id == lapic->lapic_id) {
                    lapic->proc_id = lapic_meta->ProcessorId;
                }
            }
        } else if(hdr->Type == ACPI_MADT_TYPE_IO_APIC) {
            ACPI_MADT_IO_APIC *ioapic = (ACPI_MADT_IO_APIC *) base;

            dprintf("I/O APIC ID=%x, Address=%x, IrqBase=%x\n",
                        ioapic->Id, ioapic->Address, ioapic->GlobalIrqBase);

            struct ioapic *meta = (struct ioapic *) malloc(sizeof(struct ioapic));
            meta->acpi_id = ioapic->Id;
            meta->physaddr = ioapic->Address;
            meta->irqbase = ioapic->GlobalIrqBase;

            list_insert(ioapic_list, meta, 0);
        } else if(hdr->Type == ACPI_MADT_TYPE_INTERRUPT_OVERRIDE) {
            ACPI_MADT_INTERRUPT_OVERRIDE *override = (ACPI_MADT_INTERRUPT_OVERRIDE *) base;

            dprintf("Interrupt Source Override: %d -> %d [%x]\n", override->GlobalIrq, override->SourceIrq, override->IntiFlags);

            struct override *o = (struct override *) malloc(sizeof(struct override));
            o->origirq = override->SourceIrq;
            o->newirq = override->GlobalIrq;

            uint8_t p = override->IntiFlags & 0x3;
            uint8_t t = (override->IntiFlags >> 2) & 0x3;

            // Use the flags to determine the right polarity and trigger mode.
            if(p == OVERRIDE_POLARITY_CONFORMS) {
                if((t == OVERRIDE_TRIGGER_CONFORMS) || (t == OVERRIDE_TRIGGER_EDGE)) {
                    o->trigger = 0;
                    o->polarity = 0;
                } else if(t == OVERRIDE_TRIGGER_LEVEL) {
                    o->polarity = 1;
                    o->trigger = 1;
                }
            } else {
                // Polarity doesn't conform to the bus.
                if(p == OVERRIDE_POLARITY_ACTIVELOW)
                    o->polarity = 1;
                else if(p == OVERRIDE_POLARITY_ACTIVEHIGH)
                    o->polarity = 0;

                if((t == OVERRIDE_TRIGGER_CONFORMS) || (t == OVERRIDE_TRIGGER_EDGE)) {
                    o->trigger = 0;
                } else if(t == OVERRIDE_TRIGGER_LEVEL) {
                    o->trigger = 1;
                }
            }

            tree_insert(interrupt_override, (void *) override->SourceIrq, (void *) o);
        }

        if(hdr->Length > sz) {
            sz = 0;
        } else {
            sz -= hdr->Length;
        }
        base += hdr->Length;
    }

    // Was an I/O APIC found?
    if(list_len(ioapic_list) == 0) {
        dprintf("ioapic: no I/O APIC found!\n");
        delete_list(ioapic_list);
        delete_tree(interrupt_override);
        ioapic_list = 0;
        interrupt_override = 0;
        return -1;
    }

    // Initialise all I/O APICs.
    size_t n = 0;
    struct ioapic *meta = 0;
    size_t intnum = IOAPIC_INT_BASE;
    while((meta = (struct ioapic *) list_at(ioapic_list, n++))) {
        // Map in the physical address so we can work with the I/O APIC.
        meta->mmioaddr = mmiopool_alloc(IOAPIC_MMIOSIZE, meta->physaddr);

        meta->ioapic_id = read_ioapic_reg(meta->mmioaddr, IOAPIC_REG_ID);

        uint32_t ver = read_ioapic_reg(meta->mmioaddr, IOAPIC_REG_VER);
        meta->intcount = ((ver >> IOAPIC_MAXREDIR_SHIFT) & IOAPIC_MAXREDIR_MASK) + 1;
        meta->intbase = intnum;

        // Configure the I/O APIC for all IRQ supported on this sytem.
        size_t irq = 0;
        for(; irq < meta->intcount; irq++) {

            dprintf(" configuring irq #%d\n", irq);

            uint32_t data_low = read_ioapic_reg(meta->mmioaddr, IOAPIC_REG_REDIRBASE + (irq * 2));
            uint32_t data_high = read_ioapic_reg(meta->mmioaddr, IOAPIC_REG_REDIRBASE + (irq * 2) + 1);

            // Set up the interrupt vector for this I/O APIC.
            data_low &= ~0xFF;
            data_low |= (meta->intbase + irq) & 0xFF;

            // Fixed delivery mode.
            /// \todo Change registration semantics to allow flags, so we can
            ///       make this work for low-priority interrupts etc...
            data_low &= ~(0x7 << 8);
            data_low |= 0 << 8; // Fixed mode.

            /// \todo Logical CPU destination
            data_low &= ~(0x1 << 11);

            // Mask the IRQ.
            data_low &= ~(0x1 << 16);

            // Set the destination.
            data_high &= ~(0xFF << 24);
            data_high |= (lapic->proc_id << 24);

            // Write back to the registers.
            write_ioapic_reg(meta->mmioaddr, IOAPIC_REG_REDIRBASE + (irq * 2), data_low);
            write_ioapic_reg(meta->mmioaddr, IOAPIC_REG_REDIRBASE + (irq * 2) + 1, data_high);

            // Install the interrupt.
            interrupts_trap_reg(meta->intbase + irq, handle_ioapic_irq);
        }

        // Create IRQ handlers
        meta->handlers = (struct irqhandler *) malloc(sizeof(struct irqhandler) * meta->intcount);
        memset(meta->handlers, 0, sizeof(struct irqhandler) * meta->intcount);

        // Next batch of interrupts...
        intnum += meta->intcount;

        dprintf(" I/O APIC id=%x, vaddr=%x, maxint=%d\n", meta->ioapic_id, meta->mmioaddr, meta->intcount);
    }

    // I/O APIC found, disable the 8259 PICs.
    if(madt->Flags & ACPI_MADT_PCAT_COMPAT) {
        dprintf("ioapic: disabling 8259 PICs\n");
        outb(0xA1, 0xFF);
        outb(0x21, 0xFF);
    }

    return 0;
}

void apic_interrupt_reg(int n, int leveltrig, inthandler_t handler, void *p) {
    assert(ioapic_list);

    dprintf("ioapic: installing irq for %d\n", n);

    // Find the I/O APIC for this IRQ
    size_t idx = 0;
    struct ioapic *meta = 0;
    while((meta = (struct ioapic *) list_at(ioapic_list, idx++))) {
        if((meta->irqbase <= n) && ((meta->irqbase + meta->intcount) > n)) {
            break;
        }
    }

    if(!meta) {
        dprintf("ioapic: couldn't install handler for irq %d - no I/O APIC for it!\n", n);
        return;
    }

    // Check for override.
    size_t override = n;
    struct override *oride = (struct override *) tree_search(interrupt_override, (void *) n);
    if(oride) {
        dprintf("override: IRQ %d -> %d\n", n, override);
        override = oride->newirq;
    }

    // Localise the IRQ number
    override -= meta->irqbase;

    // Install the handler.
    meta->handlers[override].handler = handler;
    meta->handlers[override].param = p;
    meta->handlers[override].actual = n;

    // Level/edge trigger, and unmask the interrupt.
    leveltrig &= 0x1;
    uint32_t data_low = read_ioapic_reg(meta->mmioaddr, IOAPIC_REG_REDIRBASE + (override * 2));
    data_low &= ~(3 << 15);
    if(!oride) {
        data_low |= leveltrig << 15;
    } else {
        data_low &= ~(1 << 13);
        data_low |= oride->polarity << 13;
        data_low |= oride->trigger << 15;
    }
    write_ioapic_reg(meta->mmioaddr, IOAPIC_REG_REDIRBASE + (override * 2), data_low);

    return;
}
