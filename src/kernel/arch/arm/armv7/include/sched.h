
#ifndef _ARM_SCHED_H
#define _ARM_SCHED_H

/**
 * An ARM scheduling context.
 * We can context switch at any time, between two kernel contexts.
 * User contexts are not relevant for context switching as we will
 * always switch within the kernel.
 */
typedef struct _arm_ctx {
} __packed context_t;

#define __halt asm volatile("wfi")

#define _CONTEXT_T_DEFINED

#endif
