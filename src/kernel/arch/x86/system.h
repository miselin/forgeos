
#ifndef _SYSTEM_H
#define _SYSTEM_H

#define PHYS_ADDR		0x100000

#define KERNEL_BASE		0xC0000000
#define HEAP_BASE		0xD0000000
#define STACK_TOP		0xFFC00000
#define STACK_SIZE		0x4000 // 16 KB

#define PAGE_SIZE		0x1000

#define log2phys(x)		(((x) - KERNEL_BASE) + PHYS_ADDR)
#define phys2log(x)		(((x) - PHYS_ADDR) + KERNEL_BASE)

#endif
