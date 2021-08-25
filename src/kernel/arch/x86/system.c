
#include <types.h>
#include <system.h>

void x86_cpuid(int code, uint32_t *a, uint32_t *b, uint32_t * c, uint32_t *d) {
    __asm__ volatile("cpuid" : "=a" (*a), "=b" (*b), "=c" (*c), "=d" (*d) : "a" (code));
}

void x86_get_msr(uint32_t msr, uint32_t *l, uint32_t *h) {
    __asm__ volatile("rdmsr" : "=a" (*l), "=d" (*h) : "c" (msr));
}

void x86_set_msr(uint32_t msr, uint32_t l, uint32_t h) {
    __asm__ volatile("wrmsr" :: "a" (l), "d" (h), "c" (msr));
}
