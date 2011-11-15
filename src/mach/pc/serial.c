
#include <stdint.h>
#include <serial.h>
#include <io.h>

#define SERIAL_RXTX		0
#define SERIAL_INTEN	1
#define SERIAL_IIFIFO	2
#define SERIAL_LCTRL	3
#define SERIAL_MCTRL	4
#define SERIAL_LSTAT	5
#define SERIAL_MSTAT	6
#define SERIAL_SCRATCH	7

static int is_connected() {
	return 1; /// \todo Implement.
}

void init_serial() {
	/// \todo Port assumption here.
	outb(0x3F8 + SERIAL_INTEN, 0); // Disable interrupts.
	outb(0x3F8 + SERIAL_LCTRL, 0x80); // Enable DLAB (set baud rate divisor)
	outb(0x3F8 + SERIAL_RXTX, 0x01); // Divisor to 3, for 115200 baud...
	outb(0x3F8 + SERIAL_INTEN, 0);
	outb(0x3F8 + SERIAL_LCTRL, 0x03); // 8 bits, no parity, one stop bit
	outb(0x3F8 + SERIAL_IIFIFO, 0xC7); // Enable FIFO, clear it
	outb(0x3F8 + SERIAL_MCTRL, 0x0B); // IRQs enabled, RTS/DSR set
	outb(0x3F8 + SERIAL_INTEN, 0x0C); // Enable all interrupts
}

void serial_write(uint8_t c) {
	if(is_connected() == 0)
		return;

	while((inb(0x3F8 + SERIAL_LSTAT) & 0x20) == 0);

	outb(0x3F8 + SERIAL_RXTX, c);
}

uint8_t serial_read() {
	if(is_connected() == 0)
		return 0;

	while((inb(0x3F8 + SERIAL_LSTAT) & 0x1) == 0);

	return inb(0x3F8 + SERIAL_RXTX);
}
