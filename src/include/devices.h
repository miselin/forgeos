
#ifndef _DEVICES_H
#define _DEVICES_H

/// Initialises basic machine-specific devices required early on. This includes
/// things such as the serial port, and timers.
#define init_devices mach_init_devices

extern void mach_init_devices();

#endif
