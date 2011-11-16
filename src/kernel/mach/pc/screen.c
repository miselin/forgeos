
#include <stdint.h>
#include <serial.h>
#include <util.h>

static uint8_t screenX = 0;
static uint8_t screenY = 0;

static uint8_t *vmem = (uint8_t *) 0xB8000;

void machine_clear_screen() {
	memset(vmem, 0, 80 * 25 * sizeof(uint16_t));
}

void machine_putc(char c) {
	serial_write((uint8_t) c);

	if(c == '\n') {
		screenX = 0;
		screenY++;
	} else {
		int offset = ((screenY * 80) + screenX) * 2;
		vmem[offset] = (unsigned char) c;
		vmem[offset + 1] = 0x07;

		screenX++;
	}

	if(screenX >= 80) {
		screenX = 0;
		screenY++;
	}

	if(screenY >= 25) {
		screenY--;

		// Scroll.
		memcpy(vmem, &vmem[80 * 2], 24 * 80 * 2);
		memset(&vmem[(80 * 2) * 24], 0, 80 * 2);
	}
}
