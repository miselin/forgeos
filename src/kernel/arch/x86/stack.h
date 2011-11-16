
#ifndef _STACK_H
#define _STACK_H

#include <stdint.h>

struct intr_stack {
	uint32_t gs, fs, es, ds;
	uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
	uint32_t intnum, ecode;
	uint32_t eip, cs, eflags, useresp, userss;
} __attribute__((packed));

#endif
