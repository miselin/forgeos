
#ifndef _ARM_SCHED_H
#define _ARM_SCHED_H

/**
 * An ARM scheduling context.
 * We can context switch at any time, between two kernel contexts.
 * User contexts are not relevant for context switching as we will
 * always switch within the kernel.
 */
typedef struct _arm_ctx {
    unative_t r4, r5, r6, r7;
    unative_t r8, r9, r10, r11;
    unative_t lr, usersp, userlr;
} __packed __aligned(8) context_t;

#define __halt asm volatile("wfi")

#define _CONTEXT_T_DEFINED

#endif
