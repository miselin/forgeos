
#include <io.h>

void panic(const char *s) {
	kprintf("PANIC: %s\n", s);
	while(1) __asm__ volatile("hlt");
}

void dlmalloc_abort() {
	panic("dlmalloc abort");
}
