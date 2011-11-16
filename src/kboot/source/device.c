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

#include <lib/string.h>

#include <device.h>
#include <fs.h>
#include <memory.h>

/** Current device. */
device_t *current_device = NULL;

/** List of all devices. */
LIST_DECLARE(device_list);

/** Look up a device according to a string.
 *
 * Looks up a device according to the given string. If the string is in the
 * form "(<name>)", then the device, will be looked up by its name. Otherwise,
 * the string will be taken as a UUID, and the device containing a filesystem
 * with that UUID will be returned.
 *
 * @param str		String for lookup.
 *
 * @return		Pointer to device structure if found, NULL if not.
 */
device_t *device_lookup(const char *str) {
	char *name = NULL;
	device_t *device;
	size_t len;

	if(str[0] == '(') {
		len = strlen(str) - 2;
		name = kmalloc(len);
		memcpy(name, str + 1, len);
		name[len] = 0;
	}

	LIST_FOREACH(&device_list, iter) {
		device = list_entry(iter, device_t, header);

		if(name) {
			if(strcmp(device->name, name) == 0) {
				return device;
			}
		} else if(device->fs && device->fs->uuid) {
			if(strcmp(device->fs->uuid, str) == 0) {
				return device;
			}
		}
	}

	return NULL;
}

/** Register a device.
 *
 * Registers a new device. Does not set the FS pointer, so this should be set
 * manually after the function returns. If this is the boot device, the caller
 * should set it as the current device itself.
 *
 * @param name		Name of the device (string will be duplicated).
 * @param data		Implementation-specific data pointer.
 *
 * @return		Pointer to created device structure.
 */
device_t *device_add(const char *name, void *data) {
	device_t *device = kmalloc(sizeof(device_t));

	list_init(&device->header);
	device->name = kstrdup(name);
	device->fs = NULL;
	device->data = data;

	list_append(&device_list, &device->header);

	return device;
}
