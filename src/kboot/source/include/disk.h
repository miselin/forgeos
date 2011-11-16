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
 * @brief		Disk device layer.
 */

#ifndef __DISK_H
#define __DISK_H

#include <device.h>
#include <system.h>

#if CONFIG_KBOOT_HAVE_DISK

struct disk;

/** Partition map iteration callback function type.
 * @param disk		Disk containing the partition.
 * @param id		ID of the partition.
 * @param lba		Start LBA.
 * @param blocks	Size in blocks.
 * @param data		Data argument passed to iterate. */
typedef void (*partition_map_iterate_cb_t)(struct disk *disk, uint8_t id, uint64_t lba,
                                           uint64_t blocks, void *data);

/** Partition map operations. */
typedef struct partition_map_ops {
	/** Iterate over the partitions on the device.
	 * @param disk		Disk to iterate over.
	 * @param cb		Callback function.
	 * @param data		Data argument to pass to the callback function.
	 * @return		Whether the device contained a partition map of
	 *			this type. */
	bool (*iterate)(struct disk *disk, partition_map_iterate_cb_t cb, void *data);
} partition_map_ops_t;

/** Define a builtin partition map type. */
#define BUILTIN_PARTITION_MAP(name) 	\
	static partition_map_ops_t name; \
	DEFINE_BUILTIN(BUILTIN_TYPE_PARTITION_MAP, name); \
	static partition_map_ops_t name

/** Operations for a disk device. */
typedef struct disk_ops {
	/** Check if a partition is the boot partition.
	 * @param disk		Disk the partition is on.
	 * @param id		ID of partition.
	 * @param lba		Block that the partition starts at.
	 * @return		Whether partition is a boot partition. */
	bool (*is_boot_partition)(struct disk *disk, uint8_t id, uint64_t lba);

	/** Read blocks from the disk.
	 * @param disk		Disk being read from.
	 * @param buf		Buffer to read into.
	 * @param lba		Block number to start reading from.
	 * @param count		Number of blocks to read.
	 * @return		Whether reading succeeded. */
	bool (*read)(struct disk *disk, void *buf, uint64_t lba, size_t count);
} disk_ops_t;

/** Structure representing a disk device. */
typedef struct disk {
	size_t block_size;		/**< Size of one block on the disk. */
	uint64_t blocks;		/**< Number of blocks on the disk. */
	disk_ops_t *ops;		/**< Pointer to operations structure. */
	union {
		struct {
			/** Implementation-specific data pointer. */
			void *data;

			/** Whether the disk is the boot disk. */
			bool boot;
		};
		struct {
			/** Parent of the partition. */
			struct disk *parent;

			/** Partition ID. */
			uint8_t id;

			/** Offset of the partition on the disk. */
			offset_t offset;
		};
	};
} disk_t;

extern bool disk_read(disk_t *disk, void *buf, size_t count, offset_t offset);
extern void disk_add(const char *name, size_t block_size, uint64_t blocks, disk_ops_t *ops,
                     void *data, bool boot);
extern disk_t *disk_parent(disk_t *disk);

extern void platform_disk_detect(void);

extern void disk_init(void);

#endif /* CONFIG_KBOOT_HAVE_DISK */
#endif /* __DISK_H */
