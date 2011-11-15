
#include <stdint.h>
#include <serial.h>
#include <pic.h>

void mach_init_devices() {
	init_pic();
	init_serial();
}
