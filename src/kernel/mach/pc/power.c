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
#include <interrupts.h>
#include <util.h>
#include <vmem.h>
#include <io.h>

#include <acpi.h>

#define ACPI_MAX_INIT_TABLES        16
static ACPI_TABLE_DESC InitTables[ACPI_MAX_INIT_TABLES];

static ACPI_TABLE_FACS *AcpiGbl_FACS;

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
            else if(new_status == ACPI_STATE_S1)
                new_status = ACPI_STATE_S0;
        } else {
            break;
        }
    }

    return new_status;
}

static void pc_acpi_wakeup() {
    dprintf("wakeup complete\n");
    while(1);
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
    dprintf("ok flags %x!\n", AcpiGbl_FACS->Flags);

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

    return 0;
}

int platform_powerman_prep(int new_state) {
    ACPI_STATUS status;

    // Set firmware wakeup vector to our wakeup code
    /// \todo Put this wakeup code below 1 MB, write it properly etc...
    AcpiSetFirmwareWakingVector((UINT32) vmem_v2p((vaddr_t) pc_acpi_wakeup));

    if(ACPI_FAILURE(status = AcpiEnterSleepStatePrep(powerman_state_to_acpi(new_state)))) {
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

    // Enter the new state.
    if(ACPI_FAILURE(status = AcpiEnterSleepState(new_state_acpi))) {
        dprintf("pc: acpi state entry failed: %s\n", AcpiFormatException(status));
        return -1;
    }

    // We have now left the state (as this code is being executed)...
    dprintf("pc: returned from sleep, leaving sleep state... ");
    AcpiLeaveSleepState(new_state_acpi);
    dprintf("ok.\n");

    // Clear wakeup vector - wake up is done.
    AcpiSetFirmwareWakingVector(0);

    return 0;
}
