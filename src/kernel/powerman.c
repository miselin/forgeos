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
#include <interrupts.h>
#include <system.h>
#include <util.h>
#include <io.h>

static void *powerman_spinlock = 0;
static void *powerman_cblist = 0;
static int current_state = 0;

int powerman_earlyinit() {
    dprintf("powerman: early init\n");
    if(platform_powerman_earlyinit() != 0) {
        current_state = POWERMAN_STATE_MAX;
        return -1;
    }

    // Needed for kernel init to install callbacks.
    powerman_spinlock = create_spinlock();
    powerman_cblist = create_list();

    return 0;
}

int powerman_init() {
    if(current_state == POWERMAN_STATE_MAX) {
        dprintf("powerman: skipping init as early init failed\n");
        return -1;
    }

    int wasints = interrupts_get();
    interrupts_disable();

    dprintf("powerman: init\n");

    current_state = POWERMAN_STATE_WORKING;

    if(platform_powerman_init() != 0) {
        dprintf("powerman: platform init failed!\n");

        delete_list(powerman_cblist);
        delete_spinlock(powerman_spinlock);

        current_state = POWERMAN_STATE_MAX;

        if(wasints)
            interrupts_enable();

        return -1;
    }

    if(wasints)
        interrupts_enable();

    return 0;
}

void powerman_installcallback(powerman_callback_t cb) {
    if(current_state == POWERMAN_STATE_MAX) {
        return;
    }

    dprintf("powerman: new callback %p\n", cb);

    spinlock_acquire(powerman_spinlock);
    list_insert(powerman_cblist, cb, -1);
    spinlock_release(powerman_spinlock);
}

void powerman_removecallback(powerman_callback_t cb) {
    if(current_state == POWERMAN_STATE_MAX) {
        return;
    }

    dprintf("powerman: new callback %p\n", cb);

    size_t n = 0;

    spinlock_acquire(powerman_spinlock);
    while((list_at(powerman_cblist, n) != (void *) cb)) n++;
    if(n < list_len(powerman_cblist))
        list_remove(powerman_cblist, n);
    spinlock_release(powerman_spinlock);
}

int powerman_enter(int new_state) {
    if(current_state == POWERMAN_STATE_MAX) {
        dprintf("powerman: can't switch to state %d as init failed\n", new_state);
        return -1;
    }

    dprintf("powerman: request to enter state %d\n", new_state);

    size_t n = 0;
    int cbr = 0;

    uint8_t wasints = interrupts_get();

    spinlock_acquire(powerman_spinlock);

    // Call all of our callbacks - almost ready to go!
    powerman_callback_t cb = NULL;
    dprintf("powerman: calling callbacks due to pending transition\n");
    while((cb = list_at(powerman_cblist, n++)) != NULL) {
        cbr = cb(new_state);
        if(cbr != 0) {
            dprintf("powerman: callback %p returned non-zero code, aborting state transition\n");
            break;
        }
    }
    n = 0;

    // Did a callback fail? Notify callbacks again of the current state (as no
    // state transition took place).
    if(cbr != 0) {
        while((cb = list_at(powerman_cblist, n++)) != NULL) cb(current_state);
        spinlock_release(powerman_spinlock);

        return cbr;
    }

    // Prepare to enter the new state, if we can.
    dprintf("powerman: preparing to enter new state... ");
    if(platform_powerman_prep(new_state) != 0) {
        dprintf("fail - notifying all callbacks and returning to state %d\n", current_state);

        while((cb = list_at(powerman_cblist, n++)) != NULL) cb(current_state);

        spinlock_release(powerman_spinlock);
        return -1;
    }
    dprintf("ok!\n");

    // Set the new state in variables.
    int old_state = current_state;
    current_state = new_state;

    // Enter the new state proper.
    dprintf("powerman: entering new state... ");
    if(platform_powerman_enter(new_state) != 0) {
        dprintf("fail - notifying all callbacks and returning to state %d\n", old_state);

        current_state = old_state;
        while((cb = list_at(powerman_cblist, n++)) != NULL) cb(current_state);

        spinlock_release(powerman_spinlock);
        return -1;
    }
    dprintf("ok!\n");

    // Made it to here - sleep state was entered and exited okay.
    // Notify callbacks of our new status, clean up, and we're done!
    current_state = POWERMAN_STATE_WORKING;
    while((cb = list_at(powerman_cblist, n++)) != NULL) cb(current_state);
    spinlock_release(powerman_spinlock);

    // Okay to bring back interrupts now if they were disabled during the
    // platform-specific transition.
    if(wasints)
        interrupts_enable();
    return 0;
}

int powerman_event(int event) {
    /// \todo Make configurable!
    switch(event) {
        case POWERMAN_EVENT_RTC:
            // Nothing to do here.
            break;

        case POWERMAN_EVENT_POWERBUTTON:
            // Bring down the system.
            powerman_enter(POWERMAN_STATE_OFF);
            break;

        case POWERMAN_EVENT_SLEEPBUTTON:
            // Put the system to sleep.
            powerman_enter(POWERMAN_STATE_STANDBY);
            break;

        default:
            dprintf("powerman: unknown event %x\n", event);
    }

    return 0;
}
