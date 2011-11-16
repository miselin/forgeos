/*
 * Copyright (C) 2011 Alex Smith
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * @file
 * @brief		Device management.
 */

#ifndef __DEVICE_H
#define __DEVICE_H

#include <lib/list.h>

struct mount;
struct disk;

/** Structure containing details of a device. */
typedef struct device {
	list_t header;			/**< Link to device list. */

	char *name;			/**< Name of the device. */
	struct mount *fs;		/**< Filesystem that resides on the device. */

	union {
#if CONFIG_KBOOT_HAVE_DISK
		struct disk *disk;	/**< Pointer to disk structure for device. */
#endif
		void *data;		/**< Data pointer to be used by non-disk devices. */
	};
} device_t;

extern device_t *current_device;

extern device_t *device_lookup(const char *str);
extern device_t *device_add(const char *name, void *data);

#endif /* __DEVICE_H */
