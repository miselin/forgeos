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

#ifndef __FS_H
#define __FS_H

#include <disk.h>
#include <system.h>

struct mount;
struct file_handle;

/** Type of a dir_iterate() callback.
 * @param name		Name of the entry.
 * @param handle	Handle to entry.
 * @param data		Data argument passed to dir_iterate().
 * @return		Whether to continue iteration. */
typedef bool (*dir_iterate_cb_t)(const char *name, struct file_handle *handle, void *arg);

/** Structure containing operations for a filesystem. */
typedef struct fs_type {
	/** Mount an instance of this filesystem.
	 * @param mount		Mount structure to fill in.
	 * @return		Whether succeeded in mounting. */
	bool (*mount)(struct mount *mount);

	/** Open a file/directory on the filesystem.
	 * @note		If not provided, a generic implementation will
	 *			be used that uses iterate().
	 * @param mount		Mount to open from.
	 * @param path		Path to file/directory to open.
	 * @return		Pointer to handle on success, NULL on failure. */
	struct file_handle *(*open)(struct mount *mount, const char *path);

	/** Close a handle.
	 * @param handle	Handle to close. */
	void (*close)(struct file_handle *handle);

	/** Read from a file.
	 * @param handle	Handle to the file.
	 * @param buf		Buffer to read into.
	 * @param count		Number of bytes to read.
	 * @param offset	Offset into the file.
	 * @return		Whether read successfully. */
	bool (*read)(struct file_handle *handle, void *buf, size_t count, offset_t offset);

	/** Get the size of a file.
	 * @param handle	Handle to the file.
	 * @return		Size of the file. */
	offset_t (*size)(struct file_handle *handle);

	/** Iterate over directory entries.
	 * @param handle	Handle to directory.
	 * @param cb		Callback to call on each entry.
	 * @param arg		Data to pass to callback.
	 * @return		Whether read successfully. */
	bool (*iterate)(struct file_handle *handle, dir_iterate_cb_t cb, void *arg);
} fs_type_t;

/** Define a builtin filesystem type. */
#define BUILTIN_FS_TYPE(name) 	\
	static fs_type_t name; \
	DEFINE_BUILTIN(BUILTIN_TYPE_FS, name); \
	static fs_type_t name

/** Structure representing a mounted filesystem. */
typedef struct mount {
	fs_type_t *type;		/**< Type structure for the filesystem. */
	struct file_handle *root;	/**< Handle to root of FS (not needed if open() implemented). */
	void *data;			/**< Implementation-specific data pointer. */
#if CONFIG_KBOOT_HAVE_DISK
	disk_t *disk;			/**< Disk that the filesystem resides on. */
#endif
	char *label;			/**< Label of the filesystem. */
	char *uuid;			/**< UUID of the filesystem. */
} mount_t;

/** Structure representing a handle to a filesystem entry. */
typedef struct file_handle {
	mount_t *mount;			/**< Mount the entry is on. */
	bool directory;			/**< Whether the entry is a directory. */
	void *data;			/**< Implementation-specific data pointer. */
	int count;			/**< Reference count. */
} file_handle_t;

extern file_handle_t *file_handle_create(mount_t *mount, bool directory, void *data);

#if CONFIG_KBOOT_HAVE_DISK
extern mount_t *fs_probe(disk_t *disk);
#endif

extern file_handle_t *file_open(mount_t *mount, const char *path);
extern void file_close(file_handle_t *handle);
extern bool file_read(file_handle_t *handle, void *buf, size_t count, offset_t offset);
extern offset_t file_size(file_handle_t *handle);

extern bool dir_iterate(file_handle_t *handle, dir_iterate_cb_t cb, void *arg);

#endif /* __FS_H */
