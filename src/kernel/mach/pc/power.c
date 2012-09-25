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

#include <powerman.h>
#include <system.h>
#include <interrupts.h>
#include <mmiopool.h>
#include <assert.h>
#include <util.h>
#include <vmem.h>
#include <pmem.h>
#include <io.h>

#include <acpi.h>

#define ACPI_MAX_INIT_TABLES        16
static ACPI_TABLE_DESC InitTables[ACPI_MAX_INIT_TABLES];

static ACPI_TABLE_FACS *AcpiGbl_FACS;

/// Architecture-specific code, called whenever the system returns from sleep.
extern void *pc_acpi_wakeup, *pc_acpi_end;

/// Saves the CPU state before sleeping. After the system wakes up, this function
/// will return '1' - otherwise, it will return '0'.
extern int pc_acpi_save_state(void *);

extern void *pc_acpi_saveblock_addr;

static paddr_t pc_reloc_acpi_wakeup;

static void *pc_wakeup_saveblock;

extern char __begin_lowmem, __end_lowmem;

static UINT8 powerman_state_to_acpi(int status) {
    UINT8 new_status = 0;
    if(status == POWERMAN_STATE_WORKING) {
        new_status = ACPI_STATE_S0;
    } else if(status == POWERMAN_STATE_HALT) {
        new_status = ACPI_STATE_S1;
    } else if(status == POWERMAN_STATE_CPUOFF) {
        new_status = ACPI_STATE_S2;
    } else if(status == POWERMAN_STATE_STANDBY) {
        new_status = ACPI_STATE_S3;
    } else if(status == POWERMAN_STATE_HIBERNATE) {
        new_status = ACPI_STATE_S4;
    } else if(status == POWERMAN_STATE_OFF) {
        new_status = ACPI_STATE_S5;
    } else {
        new_status = ACPI_STATE_S0;
    }

    // Verify the new state is in fact supported, and handle conversions where
    // they are not. eg, S3 -> S1 if S3 not supported.
    // This helps on systems such as vmware where S0, S1, and S5 are supported,
    // and S1 is a VM suspend operation.
    while(1) {
        UINT8 a, b;
        ACPI_STATUS result = AcpiGetSleepTypeData(new_status, &a, &b);
        if(ACPI_FAILURE(result)) {
            dprintf("pc: machine doesn't support S%d\n", new_status);

            if(new_status == ACPI_STATE_S3)
                new_status = ACPI_STATE_S2;
            else if(new_status == ACPI_STATE_S2)
                new_status = ACPI_STATE_S1;
            else if(new_status == ACPI_STATE_S1) {
                new_status = ACPI_STATE_S0;
                break;
            }
        } else {
            break;
        }
    }

    return new_status;
}

static void pc_remap_wakeup_data() {
    // Allocate space for the save block.
    paddr_t sblk_p = pmem_alloc_special(PMEM_SPECIAL_FIRMWARE);
    vaddr_t sblk_v = mmiopool_alloc(PAGE_SIZE, sblk_p);

    pc_acpi_saveblock_addr = (void *) sblk_p;
    pc_wakeup_saveblock = (void *) sblk_v;

    dprintf("save block is at %llx, %x\n", sblk_p, sblk_v);

    // Wakeup function(s) and variables.
    vaddr_t wakeup_virt = (vaddr_t) &pc_acpi_wakeup;
    vaddr_t begin_lowmem = (vaddr_t) &pc_acpi_wakeup;
    vaddr_t end_lowmem = (vaddr_t) &pc_acpi_end;
    size_t wakeup_region_size = end_lowmem - begin_lowmem;

    // The section should never be bigger than one page!
    assert(wakeup_region_size <= PAGE_SIZE);

    // Copy data to the low memory area. It stays mapped as we call functions in
    // the section, and leaving it mapped means we can also access all the
    // kernel during wakeup.
    paddr_t p = pmem_alloc_special(PMEM_SPECIAL_FIRMWARE);
    vmem_map((vaddr_t) p, p, VMEM_SUPERVISOR | VMEM_READWRITE);
    memcpy((void *) p, (void *) begin_lowmem, PAGE_SIZE);

    // Unmap the current region and point it to that physical address, now that
    // the copy has completed.
    vmem_unmap(begin_lowmem);
    vmem_map(begin_lowmem, p, VMEM_SUPERVISOR | VMEM_READWRITE);

    pc_reloc_acpi_wakeup = p + (wakeup_virt & (PAGE_SIZE - 1));
}

/// 10.1.5, ACPICA Programmer's Reference.
static void *pc_acpi_device_dump(ACPI_HANDLE ObjHandle, UINT32 Level, void *Context) {
    ACPI_STATUS Status;
    ACPI_DEVICE_INFO *Info;
    ACPI_BUFFER Path;
    char Buffer[256];

    Path.Length = sizeof (Buffer);
    Path.Pointer = Buffer;

    /* Get the full path of this device and print it */
    Status = AcpiGetName(ObjHandle, ACPI_FULL_PATHNAME, &Path);
    if(ACPI_SUCCESS(Status)) {
        dprintf("%s\n", Path.Pointer);
    }

    /* Get the device info for this device and print it */
    Status = AcpiGetObjectInfo(ObjHandle, &Info);
    if(ACPI_SUCCESS(Status)) {
        char buf[512] = {0};
        if(Info->Valid & ACPI_VALID_HID)
            sprintf(buf, "%s hwid is %s", buf, Info->HardwareId.String);
        if(Info->Valid & ACPI_VALID_STA)
            sprintf(buf, "%s status is %x", buf, Info->CurrentStatus);
        if(Info->Valid & ACPI_VALID_ADR)
            sprintf(buf, "%s adr is %llx", buf, Info->Address);

        if(buf[0])
            dprintf(" %s\n", buf);
    }

    return NULL;
}

// Handle events in the system.
static UINT32 pc_acpi_handle_event(void *Context) {
    ACPI_STATUS status;
    ACPI_EVENT_STATUS event_status = 0;

    dprintf("pc: acpi fixed event %x!\n", Context);

    size_t event = (size_t) Context;

    if(event == ACPI_EVENT_RTC) {
        powerman_event(POWERMAN_EVENT_RTC);
        return 0;
    } else if(event == ACPI_EVENT_POWER_BUTTON) {
        dprintf("pc: power button pushed\n");
        powerman_event(POWERMAN_EVENT_POWERBUTTON);
    } else if(event == ACPI_EVENT_SLEEP_BUTTON) {
        dprintf("pc: sleep button pushed\n");
        powerman_event(POWERMAN_EVENT_SLEEPBUTTON);
    }

    return 0;
}

int platform_powerman_earlyinit() {
    // Can we find the ACPI root pointer at all?
    ACPI_SIZE tmp = 0;
    if(ACPI_FAILURE(AcpiFindRootPointer(&tmp))) {
        dprintf("pc: RDSP not found, acpi possibly broken or not present\n");
        return -1;
    }

    dprintf("pc: acpi tables init... ");
    if(ACPI_FAILURE(AcpiInitializeTables(InitTables, ACPI_MAX_INIT_TABLES, FALSE))) {
        dprintf("fail!\n");
        return -1;
    }
    dprintf("ok!\n");

    return 0;
}

int platform_powerman_init() {
    ACPI_STATUS status;
    ACPI_OBJECT_LIST params;
    ACPI_OBJECT obj;

    dprintf("pc: acpi subsystem init... ");
    if(ACPI_FAILURE(AcpiInitializeSubsystem())) {
        dprintf("fail!\n");
        return -1;
    }
    dprintf("ok!\n");

    dprintf("pc: reallocating tables... ");
    if(ACPI_FAILURE(AcpiReallocateRootTable())) {
        dprintf("fail!\n");
        return -1;
    }
    dprintf("ok!\n");

    dprintf("pc: acpi load tables... ");
    if(ACPI_FAILURE(AcpiLoadTables())) {
        dprintf("fail!\n");
        return -1;
    }
    dprintf("ok!\n");

    // Load tables we can use later...
    dprintf("pc: loading FACS for future reference...\n");
    if(ACPI_FAILURE(AcpiGetTable(ACPI_SIG_FACS, 0, ACPI_CAST_INDIRECT_PTR(ACPI_TABLE_HEADER, &AcpiGbl_FACS)))) {
        dprintf("fail!\n");
        return -1;
    }
    dprintf("ok!\n");

    dprintf("pc: enabling acpi... ");
    if(ACPI_FAILURE(AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION))) {
        dprintf("fail!\n");
        return -1;
    }
    dprintf("ok!\n");

    dprintf("pc: initializing acpi objects... ");
    if(ACPI_FAILURE(AcpiInitializeObjects(ACPI_FULL_INITIALIZATION))) {
        dprintf("fail!\n");
        return -1;
    }

    // Switch to APIC mode if the control method is available.
    dprintf("pc: acpi using APIC mode\n");
    obj.Type = ACPI_TYPE_INTEGER;
    obj.Integer.Value = 1; // APIC mode.
    params.Count = 1;
    params.Pointer = &obj;
    status = AcpiEvaluateObject(NULL, "\\_PIC", &params, NULL);
    if(ACPI_FAILURE(status) && (status != AE_NOT_FOUND)) {
        dprintf("couldn't set PIC mode: %s\n", AcpiFormatException(status));
    } else {
        dprintf("ok %d!\n", status);
    }

    // Set up wakeup vectors and prepare state save block.
    dprintf("pc: moving wakeup vector to low memory... ");
    pc_remap_wakeup_data();
    dprintf("ok!\n");

    // Dump the ACPI namespace.
    dprintf("pc: acpi device dump:\n");
    ACPI_HANDLE SysBusHandle;
    if(ACPI_SUCCESS(AcpiGetHandle(0, ACPI_NS_ROOT_PATH, &SysBusHandle))) {
        AcpiWalkNamespace(ACPI_TYPE_DEVICE, SysBusHandle, ~0, pc_acpi_device_dump, NULL, NULL, NULL);
    }
    dprintf("ok!\n");

    // Finish GPE init.
    dprintf("pc: acpi updating all GPEs\n");
    if(ACPI_FAILURE(AcpiUpdateAllGpes())) {
        dprintf("fail!\n");
        return -1;
    }
    AcpiEnableAllRuntimeGpes();
    dprintf("ok!\n");

    // Install event handlers.
    dprintf("pc: acpi installing event handlers... ");
    if((AcpiGbl_FADT.Flags & ACPI_FADT_POWER_BUTTON) == 0) {
        AcpiInstallFixedEventHandler(ACPI_EVENT_POWER_BUTTON,
                                        pc_acpi_handle_event,
                                        (void *) ACPI_EVENT_POWER_BUTTON);
        AcpiEnableEvent(ACPI_EVENT_POWER_BUTTON, 0);
    }
    if((AcpiGbl_FADT.Flags & ACPI_FADT_SLEEP_BUTTON) == 0) {
        AcpiInstallFixedEventHandler(ACPI_EVENT_SLEEP_BUTTON,
                                        pc_acpi_handle_event,
                                        (void *) ACPI_EVENT_SLEEP_BUTTON);
        AcpiEnableEvent(ACPI_EVENT_SLEEP_BUTTON, 0);
    }
    dprintf("ok!\n");

    return 0;
}

int platform_powerman_prep(int new_state) {
    ACPI_STATUS status;
    UINT8 acpi_state = powerman_state_to_acpi(new_state);

    // Preparing for S0? Yeah, that's an error.
    if(acpi_state == ACPI_STATE_S0) {
        dprintf("pc: attempt to prepare to enter S0, likely an unsupported state\n");
        return -1;
    }

    // Set firmware wakeup vector to our wakeup code
    /// \todo Put this wakeup code below 1 MB, write it properly etc...
    if(acpi_state < ACPI_STATE_S5) {
        dprintf("pc: setting waking vector to %llx\n", pc_reloc_acpi_wakeup);
        AcpiSetFirmwareWakingVector((UINT32) pc_reloc_acpi_wakeup);
    }

    if(ACPI_FAILURE(status = AcpiEnterSleepStatePrep(acpi_state))) {
        dprintf("pc: acpi state prep failed: %s\n", AcpiFormatException(status));
        return -1;
    } else {
        return 0;
    }
}

int platform_powerman_enter(int new_state) {
    ACPI_STATUS status;
    UINT8 new_state_acpi = powerman_state_to_acpi(new_state);

    // Hibernate?
    if(new_state_acpi == ACPI_STATE_S4) {
        // Firmware S4bios support?
        if(AcpiGbl_FACS &&
            (AcpiGbl_FACS->Flags & (ACPI_FACS_S4_BIOS_PRESENT)) &&
            AcpiGbl_FADT.S4BiosRequest) {
            dprintf("S4bios supported but not yet implemented\n");
            return -1;
        } else {
            dprintf("pc: S4bios not supported by this hardware\n");
            dprintf("pc: OSPM memory save not yet implemented\n");
            return -1;
        }
    }

    interrupts_disable();
    if((new_state_acpi >= ACPI_STATE_S1) && (new_state_acpi <= ACPI_STATE_S3)) {
        // Flush all caches before entering S1, S2, or S3 (16.1.6 ACPI 5.0)
        ACPI_FLUSH_CPU_CACHE();
    }

    // Save state before we enter the new sleep state.
    dprintf("pc: acpi saving state before entering new sleep state\n");
    if(pc_acpi_save_state(pc_wakeup_saveblock) == 0) {
        dprintf("ok!\n");

        // Enter the new state.
        if(ACPI_FAILURE(status = AcpiEnterSleepState(new_state_acpi))) {
            dprintf("pc: acpi state entry failed: %s\n", AcpiFormatException(status));
            return -1;
        }
    }

    // We have now left the state (as this code is being executed)...
    dprintf("pc: returned from sleep, leaving sleep state... ");
    AcpiLeaveSleepState(new_state_acpi);
    dprintf("ok.\n");

    // Clear wakeup vector - wake up is done.
    AcpiSetFirmwareWakingVector(0);

    return 0;
}
