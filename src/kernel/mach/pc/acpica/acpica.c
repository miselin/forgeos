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

#include <system.h>
#include <types.h>
#include <sched.h>
#include <spinlock.h>
#include <interrupts.h>
#include <semaphore.h>
#include <mmiopool.h>
#include <compiler.h>
#include <stdarg.h>
#include <malloc.h>
#include <sleep.h>
#include <panic.h>
#include <vmem.h>
#include <io.h>

#include <acpi.h>

#define ACPICA_OSL_VERBOSE

#ifndef ACPICA_OSL_VERBOSE
#undef dprintf
#define dprintf(a, ...)
#endif

// Support functions for ACPICA.

ACPI_STATUS AcpiOsInitialize() {
    dprintf("acpi: OS init\n");
    return AE_OK;
}

ACPI_STATUS AcpiOsTerminate() {
    dprintf("acpi: OS terminate\n");
    return AE_OK;
}

ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer()
{
    ACPI_SIZE Ret;
    AcpiFindRootPointer(&Ret);
    dprintf("acpi: get root pointer (at=%x)\n", Ret);
    return Ret;
}

ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *PredefinedObject __unused, ACPI_STRING *NewValue) {
    dprintf("acpi: predefined override\n");
    *NewValue = 0;
    return AE_OK;
}

ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER *ExistingTable __unused, ACPI_TABLE_HEADER **NewTable) {
    dprintf("acpi: table override\n");
    *NewTable = 0;
    return AE_OK;
}

ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER *ExistingTable __unused, ACPI_PHYSICAL_ADDRESS *NewAddress, UINT32 *NewTableLength __unused) {
    dprintf("acpi: physical table override\n");
    *NewAddress = 0;
    return AE_OK;
}

void *AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress, ACPI_SIZE Length) {
    dprintf("acpi: map memory %d bytes -> %x [end %x]\n", Length, PhysicalAddress, PhysicalAddress + Length);
    ACPI_PHYSICAL_ADDRESS addend = PhysicalAddress & (PAGE_SIZE - 1);
    void *addr = mmiopool_alloc(Length, PhysicalAddress);
    dprintf("acpi: mapped at %x\n", addr);
    void *ret = (void *) (((uintptr_t) addr) + addend);
    dprintf("acpi: map memory returns %p\n", ret);
    return ret;
}

void AcpiOsUnmapMemory(void *where, ACPI_SIZE length) {
    dprintf("acpi: unmap memory from %p, %d bytes\n", where, length);
    mmiopool_dealloc(where);
}

ACPI_STATUS AcpiOsGetPhysicalAddress(void *LogicalAddress, ACPI_PHYSICAL_ADDRESS *PhysicalAddress) {
    dprintf("acpi: v2p %p\n", LogicalAddress);
    *PhysicalAddress = (ACPI_PHYSICAL_ADDRESS) vmem_v2p((vaddr_t) LogicalAddress);
    return AE_OK;
}

void *AcpiOsAllocate(ACPI_SIZE Size) {
    dprintf("acpi: allocate %d bytes\n", Size);
    return malloc(Size);
}

void AcpiOsFree(void *Memory) {
    dprintf("acpi: free %p\n", Memory);
    free(Memory);
}

BOOLEAN AcpiOsReadable(void *Memory, ACPI_SIZE Length) {
    dprintf("acpi: readable %p, %d bytes\n", Memory, Length);
    ACPI_SIZE i = 0;
    for(; i < Length; i += PAGE_SIZE) {
        if(!vmem_ismapped(((vaddr_t) Memory) + (i * PAGE_SIZE))) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOLEAN AcpiOsWritable(void *Memory, ACPI_SIZE Length) {
    dprintf("acpi: writable %p, %d bytes\n", Memory, Length);
    /// \todo no vmem_getflags or similar, so this is not correct!
    return AcpiOsReadable(Memory, Length);
}

ACPI_THREAD_ID AcpiOsGetThreadId() {
    dprintf("acpi: get thread id\n");
    return (ACPI_THREAD_ID) sched_current_thread();
}

ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE Type, ACPI_OSD_EXEC_CALLBACK Function, void *Context) {
    dprintf("acpi: execute %d, %p, %p\n", Type, Function, Context);
    struct process *parent = sched_current_thread()->parent;
    struct thread *check = create_thread(parent, THREAD_PRIORITY_HIGH, (thread_entry_t) Function, 0, 0, Context);
    if(check) {
        thread_wake(check);
        return AE_OK;
    } else {
        return AE_NOT_EXIST;
    }
}

void AcpiOsWaitEventsComplete() {
    dprintf("acpi: wait for events to complete\n");
}

void AcpiOsSleep(UINT64 Milliseconds) {
    dprintf("acpi: sleep %lld milliseconds\n", Milliseconds);
    sleep_ms((uint32_t) Milliseconds);
}

void AcpiOsStall(UINT32 Microseconds) {
    dprintf("acpi: stall %d microseconds\n", Microseconds);
    /// \todo write me.
}

ACPI_STATUS AcpiOsCreateMutex(ACPI_MUTEX *OutHandle) {
    dprintf("acpi: create mutex\n");
    *OutHandle = create_semaphore(1, 1);
    return AE_OK;
}

void AcpiOsDeleteMutex(ACPI_MUTEX Handle) {
    dprintf("acpi: delete mutex\n");
    delete_semaphore(Handle);
}

ACPI_STATUS AcpiOsAcquireMutex(ACPI_MUTEX Handle, UINT16 Timeout) {
    dprintf("acpi: acquire mutex with %d ms timeout\n", Timeout);
    semaphore_acquire(Handle, 1);
    return AE_OK;
}

void AcpiOsReleaseMutex(ACPI_MUTEX Handle) {
    dprintf("acpi: release mutex\n");
    semaphore_release(Handle, 1);
}

ACPI_STATUS AcpiOsCreateSemaphore(UINT32 MaxUnits, UINT32 InitialUnits, ACPI_SEMAPHORE *OutHandle) {
    dprintf("acpi: create semaphore (max=%d, initial=%d)\n", MaxUnits, InitialUnits);
    *OutHandle = create_semaphore(MaxUnits, InitialUnits);
    return AE_OK;
}

ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle) {
    dprintf("acpi: delete semaphore\n");
    delete_semaphore(Handle);
    return AE_OK;
}

ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units, UINT16 Timeout) {
    dprintf("acpi: wait semaphore for %d units, %d ms timeout\n", Units, Timeout);
    semaphore_acquire(Handle, Units);
    return AE_OK;
}

ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units) {
    dprintf("acpi: signal semaphore (%d units)\n", Units);
    semaphore_release(Handle, Units);
    return AE_OK;
}

ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK *OutHandle) {
    dprintf("acpi: create spinlock\n");
    *OutHandle = create_spinlock();
    return AE_OK;
}

void AcpiOsDeleteLock(ACPI_HANDLE Handle) {
    dprintf("acpi: destroy spinlock\n");
    delete_spinlock(Handle);
}

ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle) {
    dprintf("acpi: acquire spinlock\n");
    spinlock_acquire(Handle);
    return 0;
}

void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags __unused) {
    dprintf("acpi: release spinlock\n");
    spinlock_release(Handle);
}

static ACPI_OSD_HANDLER ServiceRoute = 0;

int acpi_inthandler(struct intr_stack *p, void *d) {
    if(ServiceRoute)
        ServiceRoute(d);
    return 0;
}


ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 InterruptLevel, ACPI_OSD_HANDLER Handler, void *Context) {
    dprintf("acpi: install interrupt %d handler\n", InterruptLevel);

    ServiceRoute = Handler;

    interrupts_irq_reg(InterruptLevel, 0, acpi_inthandler, Context);
    return AE_OK;
}

ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER Handler __unused) {
    dprintf("acpi: remove interrupt handler %d\n", InterruptNumber);
    ServiceRoute = 0;
    return AE_OK;
}

void AcpiOsPrintf(const char *fmt, ...) {
    dprintf("acpi: printf\n");
    va_list args;
    va_start(args, fmt);
    AcpiOsVprintf(fmt, args);
    va_end(args);
}

void AcpiOsVprintf(const char *fmt, va_list args) {
    dprintf("acpi: vprintf\n");
    char buf[512];
    vsprintf(buf, fmt, args);
    dputs(buf);
    dputs("\n");
}

ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32 *Value, UINT32 Width) {
    dprintf("acpi: %d bit read port %d\n", Width, Address);
    if(Width == 8)
        *Value = inb(Address);
    else if(Width == 16)
        *Value = inw(Address);
    else
        *Value = inl(Address);
    return AE_OK;
}

ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width) {
    dprintf("acpi %d bit write port %d\n", Width, Address);
    if(Width == 8)
        outb(Address, (uint8_t) Value);
    else if(Width == 16)
        outw(Address, (uint16_t) Value);
    else
        outl(Address, Value);
    return AE_OK;
}

ACPI_STATUS AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 *Value, UINT32 Width) {
    dprintf("acpi: read memory at %llx\n", Address);
    if(Width == 8) {
        uint8_t *p = (uint8_t *) Address;
        *Value = *p;
    } else if(Width == 16) {
        uint16_t *p = (uint16_t *) Address;
        *Value = *p;
    } else if(Width == 32) {
        uint32_t *p = (uint32_t *) Address;
        *Value = *p;
    } else {
        uint64_t *p = (uint64_t *) Address;
        *Value = *p;
    }

    return AE_OK;
}

ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 Value, UINT32 Width) {
    dprintf("acpi: write memory at %llx\n", Address);
    if(Width == 8) {
        uint8_t *p = (uint8_t *) Address;
        *p = Value;
    } else if(Width == 16) {
        uint16_t *p = (uint16_t *) Address;
        *p = Value;
    } else if(Width == 32) {
        uint32_t *p = (uint32_t *) Address;
        *p = Value;
    } else {
        uint64_t *p = (uint64_t *) Address;
        *p = Value;
    }

    return AE_OK;
}

ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID *PciId, UINT32 Register, UINT64 *Value, UINT32 Width) {
    dprintf("acpi: read pci config\n");
    return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS AcpiOsWritePciConfiguration(ACPI_PCI_ID *PciId, UINT32 Register, UINT64 Value, UINT32 Width) {
    dprintf("acpi: write pci config\n");
    return AE_NOT_IMPLEMENTED;
}

UINT64 AcpiOsGetTimer() {
    dprintf("acpi: get timer\n");
    UINT64 ret;
    __asm__ volatile("rdtsc" : "=A" (ret));
    return ret;
}

ACPI_STATUS AcpiOsSignal(UINT32 Function, void *Info __unused) {
    dprintf("acpi: signal to OS - %d\n", Function);
    if(Function == ACPI_SIGNAL_BREAKPOINT)
        __asm__ volatile("int $3");
    else if(Function == ACPI_SIGNAL_FATAL)
        panic("acpi fatal signal");
    return AE_OK;
}
