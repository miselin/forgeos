#ifndef _IO_H
#define _IO_H

#include <stdint.h>

/// Print a single character to the screen abstraction provided by the machine.
#define putc(c)	machine_putc(c)

/// Clear the screen (abstracted by the machine).
#define clrscr	machine_clear_screen

extern void machine_putc(char c);
extern void machine_clear_screen();

extern void puts(const char *s);

extern void serial_puts(const char *s);

#ifdef DEBUG
extern int dprintf(const char *fmt, ...);
#else
#define dprintf(a, ...)
#endif

extern int kprintf(const char *fmt, ...);
extern int sprintf(char * s, const char *fmt, ...);

// I don't like doing this, but x86 just has to be different...
#ifdef X86
extern uint8_t inb(uint16_t port);
extern uint16_t inw(uint16_t port);
extern uint32_t inl(uint16_t port);

extern void outb(uint16_t port, uint8_t val);
extern void outw(uint16_t port, uint16_t val);
extern void outl(uint16_t port, uint32_t val);
#endif

#endif
