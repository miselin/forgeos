
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
} __packed context_t;

#define _CONTEXT_T_DEFINED

#endif
