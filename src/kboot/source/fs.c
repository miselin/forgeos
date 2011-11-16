/*
 * Copyright (C) 2010-2011 Alex Smith
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
 * @brief		Filesystem functions.
 */

#include <lib/string.h>
#include <lib/utility.h>

#include <assert.h>
#include <fs.h>
#include <memory.h>

/** Create a file handle.
 * @param path		Path to filesystem entry.
 * @param mount		Mount the entry resides on.
 * @param directory	Whether the entry is a directory.
 * @param data		Implementation-specific data pointer.
 * @return		Pointer to handle structure. */
file_handle_t *file_handle_create(mount_t *mount, bool directory, void *data) {
	file_handle_t *handle = kmalloc(sizeof(file_handle_t));
	handle->mount = mount;
	handle->directory = directory;
	handle->data = data;
	handle->count = 1;
	return handle;
}

#if CONFIG_KBOOT_HAVE_DISK
/** Probe a disk for filesystems.
 * @param disk		Disk to probe.
 * @return		Pointer to mount if detected, NULL if not. */
mount_t *fs_probe(disk_t *disk) {
	mount_t *mount;

	mount = kmalloc(sizeof(mount_t));

	BUILTIN_ITERATE(BUILTIN_TYPE_FS, fs_type_t, type) {
		memset(mount, 0, sizeof(mount_t));
		mount->disk = disk;
		mount->type = type;
		if(mount->type->mount(mount)) {
			return mount;
		}
	}

	kfree(mount);
	return NULL;
}
#endif

/** Structure containing data for file_open(). */
typedef struct file_open_data {
	const char *name;		/**< Name of entry being searched for. */
	file_handle_t *handle;		/**< Handle to found entry. */
} file_open_data_t;

/** Directory iteration callback for file_open().
 * @param name		Name of entry.
 * @param handle	Handle to entry.
 * @param _data		Pointer to data structure.
 * @return		Whether to continue iteration. */
static bool file_open_cb(const char *name, file_handle_t *handle, void *_data) {
	file_open_data_t *data = _data;

	if(strcmp(name, data->name) == 0) {
		handle->count++;
		data->handle = handle;
		return false;
	} else {
		return true;
	}
}

/** Open a handle to a file/directory.
 * @param mount		Mount to open from. If NULL, current device will be used.
 * @param path		Path to entry to open.
 * @return		Pointer to handle on success, NULL on failure. */
file_handle_t *file_open(mount_t *mount, const char *path) {
	char *dup, *orig, *tok;
	file_open_data_t data;
	file_handle_t *handle;

	if(!mount) {
		if(!(mount = current_device->fs)) {
			return NULL;
		}
	}

	/* Use the provided open() implementation if any. */
	if(mount->type->open) {
		return mount->type->open(mount, path);
	}

	assert(mount->type->iterate);

	/* Strip leading / characters from the path. */
	while(*path == '/') {
		path++;
	}

	assert(mount->root);
	handle = mount->root;
	handle->count++;

	/* Loop through each element of the path string. The string must be
	 * duplicated so that it can be modified. */
	dup = orig = kstrdup(path);
	while(true) {
		tok = strsep(&dup, "/");
		if(tok == NULL) {
			/* The last token was the last element of the path
			 * string, return the node we're currently on. */
			kfree(orig);
			return handle;
		} else if(!handle->directory) {
			/* The previous node was not a directory: this means
			 * the path string is trying to treat a non-directory
			 * as a directory. Reject this. */
			file_close(handle);
			kfree(orig);
			return NULL;
		} else if(!tok[0]) {
			/* Zero-length path component, do nothing. */
			continue;
		}

		/* Search the directory for the entry. */
		data.name = tok;
		data.handle = NULL;
		if(!mount->type->iterate(handle, file_open_cb, &data) || !data.handle) {
			file_close(handle);
			kfree(orig);
			return NULL;
		}

		file_close(handle);
		handle = data.handle;
	}
}

/** Close a handle.
 * @param handle	Handle to close. */
void file_close(file_handle_t *handle) {
	if(--handle->count == 0) {
		if(handle->mount->type->close) {
			handle->mount->type->close(handle);
		}
		kfree(handle);
	}
}

/** Read from a file.
 * @param handle	Handle to file to read from.
 * @param buf		Buffer to read into.
 * @param count		Number of bytes to read.
 * @param offset	Offset in the file to read from.
 * @return		Whether the read was successful. */
bool file_read(file_handle_t *handle, void *buf, size_t count, offset_t offset) {
	assert(!handle->directory);

	if(!count) {
		return true;
	}

	return handle->mount->type->read(handle, buf, count, offset);
}

/** Get the size of a file.
 * @param handle	Handle to the file.
 * @return		Size of the file. */
offset_t file_size(file_handle_t *handle) {
	assert(!handle->directory);
	return handle->mount->type->size(handle);
}

/** Iterate over entries in a directory.
 * @param handle	Handle to directory.
 * @param cb		Callback to call on each entry.
 * @param arg		Data to pass to callback.
 * @return		Whether read successfully. */
bool dir_iterate(file_handle_t *handle, dir_iterate_cb_t cb, void *arg) {
	assert(handle->directory);
	return handle->mount->type->iterate(handle, cb, arg);
}
