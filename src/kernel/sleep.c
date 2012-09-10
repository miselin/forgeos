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

#include <sleep.h>
#include <sched.h>
#include <util.h>
#include <timer.h>
#include <malloc.h>
#include <io.h>

static void *tlist = 0;

static int timer_installed = 0;

#define MS_TO_TICKS 1000000

struct sleepinfo {
    struct thread *t;
    uint64_t tc;
};

int sleep_timer_tick(uint64_t ticks) {
    if(tlist == NULL)
        return 0;

    // Test the list.
    struct sleepinfo *s = 0;
    size_t n = 0;
    int ret = 0;
    while((s = list_at(tlist, n)) != NULL) {
        if(s->tc < ticks)
            s->tc = 0;
        else
            s->tc -= ticks;

        if(!s->tc) {
            thread_wake(s->t);

            free(s);
            list_remove(tlist, n);

            // Will cause a reschedule to that particular thread.
            ret = 1;
        } else {
            n++;
        }
    }

    return ret;
}

void sleep_ms(uint32_t ms) {
    if(!tlist)
        tlist = create_list();

    struct thread *c = sched_current_thread();
    struct sleepinfo *s = (struct sleepinfo *) malloc(sizeof(struct sleepinfo));
    s->t = c;
    s->tc = ms * 1000000;

    list_insert(tlist, s, 0);

    if(!timer_installed) {
        install_timer(sleep_timer_tick, ((1 << TIMERRES_SHIFT) | TIMERRES_MILLI), TIMERFEAT_PERIODIC);
        timer_installed = 1;
    }

    thread_sleep();
}
