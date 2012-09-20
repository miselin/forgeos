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
#include <spinlock.h>
#include <util.h>
#include <io.h>

#include <acpi.h>

#define ACPI_MAX_INIT_TABLES        16
static ACPI_TABLE_DESC InitTables[ACPI_MAX_INIT_TABLES];

static UINT8 powerman_state_to_acpi(int status) {
    if(status == POWERMAN_STATE_WORKING) {
        return ACPI_STATE_S0;
    } else if(status == POWERMAN_STATE_HALT) {
        return ACPI_STATE_S1;
    } else if(status == POWERMAN_STATE_CPUOFF) {
        return ACPI_STATE_S2;
    } else if(status == POWERMAN_STATE_STANDBY) {
        return ACPI_STATE_S3;
    } else if(status == POWERMAN_STATE_HIBERNATE) {
        return ACPI_STATE_S4;
    } else if(status == POWERMAN_STATE_OFF) {
        return ACPI_STATE_S5;
    } else {
        return ACPI_STATE_S0;
    }
}

int platform_powerman_earlyinit() {
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
    if(ACPI_FAILURE(AcpiEnterSleepStatePrep(powerman_state_to_acpi(new_state)))) {
        return -1;
    } else {
        return 0;
    }
}

int platform_powerman_enter(int new_state) {
    if(ACPI_FAILURE(AcpiEnterSleepState(powerman_state_to_acpi(new_state)))) {
        return -1;
    } else {
        return 0;
    }
}
