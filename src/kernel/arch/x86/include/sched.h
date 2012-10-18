
#ifndef _X86_SCHED_H
#define _X86_SCHED_H

/**
 * An x86 scheduling context.
 * We can context switch at any time, between two kernel contexts.
 * User contexts are not relevant for context switching as we will
 * always switch within the kernel. An IRET from an interrupt will
 * restore enough state to get back to a user context.
 */
typedef struct _x86_ctx {
	uint32_t edi, esi, ebx;
	uint32_t ebp, esp, eip;
    uint32_t eflags;

    uint32_t stackbase;
    uint32_t stackispool;
} __packed context_t;

#define __halt __asm__ __volatile__ ("hlt")
#define __spin __asm__ __volatile__ ("pause")

#define _CONTEXT_T_DEFINED

#endif
