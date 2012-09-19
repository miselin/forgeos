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

#include <semaphore.h>
#include <spinlock.h>
#include <malloc.h>
#include <sched.h>
#include <util.h>
#include <io.h>

struct semaphore {
    size_t count;
    size_t max;

    void *spinlock;

    void *tqueue;

    int cleanup;
};

void *create_semaphore(size_t max, size_t initial) {
    struct semaphore *ret = (struct semaphore *) malloc(sizeof(struct semaphore));

    ret->count = initial;
    ret->max = max;

    ret->spinlock = create_spinlock();
    ret->tqueue = create_queue();

    ret->cleanup = 0;

    return (void *) ret;
}

void delete_semaphore(void *sem) {
    if(sem == NULL)
        return;

    struct semaphore *s = (struct semaphore *) sem;

    // Wait for any threads currently in the acquire/release critical section
    spinlock_acquire(s->spinlock);

    int was_empty = queue_empty(s->tqueue);
    while(!queue_empty(s->tqueue)) {
        struct thread *t = queue_pop(s->tqueue);
        thread_wake(t);
    }

    delete_queue(s->tqueue);

    spinlock_release(s->spinlock);
    delete_spinlock(s->spinlock);

    if(was_empty)
        free(s);
    else
        s->cleanup = 1;
}

void semaphore_acquire(void *sem, size_t count) {
    if(sem == NULL)
        return;

    // Stop people being retarded and hacking around blocking.
    if(count == 0)
        count = 1;

    // Attempt a quick acquire.
    if(semaphore_tryacquire(sem, count))
        return;

    struct semaphore *s = (struct semaphore *) sem;

    // Ensure that the semaphore's maximum is honored.
    if(count > s->max)
        count = s->max;

    while(1) {
        // Enter the acquire critical section.
        spinlock_acquire(s->spinlock);

        // Can we acquire?
        if(semaphore_tryacquire(sem, count)) {
            spinlock_release(s->spinlock);
            return;
        }

        // Push onto the pending queue...
        queue_push(s->tqueue, sched_current_thread());

        // Exit the critical section...
        spinlock_release(s->spinlock);

        // Sleep!
        thread_sleep();

        // If we were woken up by a cleanup request, clean up.
        if(s->cleanup) {
            /// \todo clean up, safely.
            return;
        }
    }
}

/// Try and acquire the given count on the given semaphore.
int semaphore_tryacquire(void *sem, size_t count) {
    if(sem == NULL)
        return 0;

    // Stop people being retarded and hacking around blocking.
    if(count == 0)
        count = 1;

    struct semaphore *s = (struct semaphore *) sem;

    // Ensure that the semaphore's maximum is honored.
    if(count > s->max)
        count = s->max;

    size_t value = s->count;
    if(count > value)
        return 0;

    if(atomic_bool_compare_and_swap(&s->count, value, value - count))
        return 1;
    else
        return 0;
}

void semaphore_release(void *sem, size_t count) {
    if(sem == NULL)
        return;

    // Stop people being retarded and hacking around blocking.
    if(count == 0)
        count = 1;

    struct semaphore *s = (struct semaphore *) sem;

    // Ensure that the semaphore's maximum is honored.
    if(count > s->max)
        count = s->max;

    // Enter the acquire critical section.
    spinlock_acquire(s->spinlock);

    // Release operation.
    s->count += count;
    if(s->count > s->max)
        s->count = s->max;

    // Wake up threads, they will each check to see if they can now complete
    // their acquire.
    while(!queue_empty(s->tqueue)) {
        struct thread *t = queue_pop(s->tqueue);
        thread_wake(t);
    }

    // Done!
    spinlock_release(s->spinlock);
}
