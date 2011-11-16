/*
 * Copyright (C) 2009-2011 Alex Smith
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
 * @brief		MSDOS partition table scanner.
 */

#include <lib/utility.h>

#include <memory.h>
#include <system.h>

#include "msdos.h"

/** Iterate over the partitions on a device.
 * @param disk		Disk to iterate over.
 * @param cb		Callback function.
 * @param data		Data argument to pass to the callback function.
 * @return		Whether the device contained an MSDOS partition table. */
static bool msdos_partition_iterate(disk_t *disk, partition_map_iterate_cb_t cb, void *data) {
	msdos_mbr_t *mbr = kmalloc(disk->block_size);
	msdos_part_t *part;
	size_t i;

	/* Read in the MBR, which is in the first block on the device. */
	if(!disk_read(disk, mbr, sizeof(msdos_mbr_t), 0) || mbr->signature != MSDOS_SIGNATURE) {
		kfree(mbr);
		return false;
	}

	/* Loop through all partitions in the table. */
	for(i = 0; i < ARRAYSZ(mbr->partitions); i++) {
		part = &mbr->partitions[i];
		if(part->type == 0 || (part->bootable != 0 && part->bootable != 0x80)) {
			continue;
		} else if(part->start_lba >= disk->blocks) {
			continue;
		} else if(part->start_lba + part->num_sects > disk->blocks) {
			continue;
		}

		dprintf("disk: found MSDOS partition %d:\n", i);
		dprintf(" type:      0x%x\n", part->type);
		dprintf(" start_lba: %u\n", part->start_lba);
		dprintf(" num_sects: %u\n", part->num_sects);

		cb(disk, i, part->start_lba, part->num_sects, data);
	}

	kfree(mbr);
	return true;
}

/** MS-DOS parition map type. */
BUILTIN_PARTITION_MAP(msdos_partition_map) = {
	.iterate = msdos_partition_iterate,
};
