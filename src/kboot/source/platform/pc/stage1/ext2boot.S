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
 * @brief		Ext2/3/4 boot sector.
 *
 * This code will load the loader binary from an Ext2/3/4 filesystem. It is
 * intended to be written to the first 1024 bytes of a partition and used as
 * the partition's boot sector.
 *
 * Since the MBR only loads 512 bytes for us, we are split into 2 sections, and
 * we load the second half early on in the main function.
 *
 * The start LBA (32-bit) of the boot partition should be written to offset 502.
 * The inode number of the loader itself should be written to offset 506.
 *
 * Current limitations:
 *  - Only uses INT13 Extensions. This is also the case with the loader itself,
 *    if anyone feels like adding support for the legacy interface, go ahead.
 *  - Uses 32-bit LBAs, meaning we only support disks up to 2TB.
 *  - Assumes EXT2_DYNAMIC_REV. The loader itself requires this at the moment,
 *    if that changes we should add support for it here too.
 *  - Only supports up to the indirect block. I doubt any greater will be
 *    needed here, unless we have to deal with very large directories or
 *    something.
 */

#include <platform/system.h>
#include <x86/asm.h>

#include "../../../fs/ext2.h"

/** Address of the stack. */
#define STACK_OFFSET		0x6000

/** Segment that inode data is loaded to. */
#define LOAD_SEGMENT		(LOADER_LOAD_ADDR >> 4)

/** Print a message. */
#define PRINT(s)		\
	movw	$s, %si; \
	call	puts

/** Print an error message and halt. */
#define ERROR(s)		\
	movw	$s, %si; \
	call 	puts; \
	hlt

#define pushad			data32 pusha
#define popad			data32 popa

.section .text
.code16

/** Main function. */
FUNCTION_START(_start)
	/* Set up the stack and other segment registers. */
	xorw	%ax, %ax
	movw	%ax, %ss
	movw	$STACK_OFFSET, %sp
	movw	%ax, %ds
	movw	%ax, %es

	PRINT(msg_loading)

	/* Save drive ID. */
	mov	%dl, bios_drive_id

	/* Read in our second half and the superblock (3 sectors total). */
	movb	$3, %cl
	xorl	%eax, %eax
	inc	%eax
	movw	$second_half, %di
	call	read_disk_sectors

	/* Check whether this is a valid Ext2 filesystem. */
	movw	(superblock + EXT2_SUPERBLOCK_OFF_MAGIC), %ax
	cmpw	$EXT2_MAGIC, %ax
	je	.Lvalid_fs

	ERROR(msg_not_ext2)
.Lvalid_fs:
	/* Compute and save the block size. Although s_log_block_size is a
	 * 32-bit field in the superblock, in reality it'll never even be
	 * greater than 10, so just treat it as a byte. */
	movl	$1024, %eax
	movb	(superblock + EXT2_SUPERBLOCK_OFF_LOG_BLOCK_SIZE), %cl
	shl	%cl, %eax
	xorl	%edx, %edx
	movl	%eax, block_size
	movl	$512, %ecx
	div	%ecx
	movl	%eax, sectors_per_block

	/* Compute and save the group table start LBA. */
	movl	(superblock + EXT2_SUPERBLOCK_OFF_FIRST_DATA_BLOCK), %ecx
	inc	%ecx
	xchg	%eax, %ecx
	mull	%ecx
	movl	%eax, group_table_offset

	/* Read in our inode. */
	movl	inode_number, %eax
	call	read_inode
	movl	(inode + EXT2_INODE_OFF_SIZE), %eax

	/* Read in the inode data. */
	call	read_inode_data

	PRINT(msg_ok)

	/* Execute the bootloader. */
	mov	bios_drive_id, %dl
	movl	partition_lba, %ebx
	xorl	%ecx, %ecx
	ljmp	$(LOADER_LOAD_ADDR >> 4), $0
FUNCTION_END(_start)

/** Read a single sector from the boot partition.
 * @param %eax		Start offset.
 * @param %es:%di	Destination buffer. */
PRIVATE_FUNCTION_START(read_disk_sector)
	/* Set count and fall through. */
	movb	$1, %cl
FUNCTION_END(read_disk_sector)

/** Read sectors from the boot partition.
 * @param %cl		Sector count.
 * @param %eax		Start offset.
 * @param %es:%di	Destination buffer. */
PRIVATE_FUNCTION_START(read_disk_sectors)
	pushad

	/* Get the real offset. */
	add	partition_lba, %eax

	/* Initialise DAP. */
	movb	$0x10, dap_size
	movb	$0, dap_reserved1
	xorb	%ch, %ch
	movw	%cx, dap_count
	movw	%di, dap_buffer_offset
	movw	%es, dap_buffer_segment
	movl	%eax, dap_lba_low
	movl	$0, dap_lba_high

	/* Perform the read. */
	movb	bios_drive_id, %dl
	movw	$dap_size, %si
	movb	$0x42, %ah
	int	$0x13
	jc	1f
	popad
	ret
1:	ERROR(msg_read_failed)
FUNCTION_END(read_disk_sectors)

/** Read an inode into the inode buffer.
 * @param %eax		Inode to read. */
PRIVATE_FUNCTION_START(read_inode)
	pushad

	/* Compute the group number in EAX, and inode table offset in EDX. */
	dec	%eax
	xorl	%edx, %edx
	divl	(superblock + EXT2_SUPERBLOCK_OFF_INODES_PER_GROUP)
	push	%eax
	movl	%edx, %eax
	movzxw	(superblock + EXT2_SUPERBLOCK_OFF_INODE_SIZE), %ecx
	mul	%ecx
	pop	%edx
	xchg	%eax, %edx
	push	%edx

	/* Need to read in the group table entry. Work out which sector of the
	 * table it is in (EAX), and the offset into that entry (EDX), and read
	 * it in. */
	movl	$EXT2_GROUP_DESC_SIZE, %ecx
	mul	%ecx
	xorl	%edx, %edx
	movl	$512, %ecx
	div	%ecx
	addl	group_table_offset, %eax
	movw	$temp_buffer, %di
	call	read_disk_sector

	/* Retrieve the inode table block, then work out the sector that the
	 * inode is in (EAX) and the offset into that sector (EDX), and read
	 * it in. */
	movl	(temp_buffer + EXT2_GROUP_DESC_OFF_INODE_TABLE)(%edx), %eax
	mull	block_size
	pop	%edx
	add	%edx, %eax
	xorl	%edx, %edx
	movl	$512, %ecx
	div	%ecx
	movw	$temp_buffer, %di
	call	read_disk_sector

	/* Copy the inode to the inode buffer. */
	cld
	movw	$EXT2_INODE_SIZE, %cx
	movw	$inode, %di
	movw	$temp_buffer, %si
	addw	%dx, %si
	rep movsb

	popad
	ret
FUNCTION_END(read_inode)

/** Read a filesystem block.
 * @param %eax		Raw block number to read.
 * @param %es:%di	Location to read to. */
PRIVATE_FUNCTION_START(read_block)
	push	%eax
	push	%ecx

	/* Check for sparse blocks (raw block number 0). */
	test	%eax, %eax
	jz	.Lsparse_block

	/* Read sectors in from the disk. */
	movl	sectors_per_block, %ecx
	mul	%ecx
	call	read_disk_sectors
	jmp	.Lret
.Lsparse_block:
	push	%es
	push	%di

	/* Fill block with zeros. */
	movl	block_size, %ecx
	xorb	%al, %al
	rep stosb

	pop	%di
	pop	%es
.Lret:
	pop	%ecx
	pop	%eax
	ret
FUNCTION_END(read_block)

/** Display a string on the screen.
 * @param %ds:%si	String to display. */
PRIVATE_FUNCTION_START(puts)
	pushw	%ax
	cld
1:	lodsb
	test	%al, %al
	jz	2f
	pushw	%bx
	movb	$0x0E, %ah
	movw	$1, %bx
	int	$0x10
	popw	%bx
	jmp	1b
2:	popw	%ax
	ret
FUNCTION_END(puts)

/** Error strings. */
msg_loading:		.asciz "Loading... "
msg_read_failed:	.asciz "Read failed!\r\n"
msg_not_ext2:		.asciz "Unrecognised FS!\r\n"
msg_ok:			.asciz "OK\r\n"

/* Pad up to the end of the first 512 bytes, with space for the partition LBA
 * and the boot signature at the end. */
. = _start + 0x1F6
partition_lba:		.long 0
inode_number:		.long 0
boot_signature:		.word 0xAA55

second_half:

/** Read all data for the inode to LOADER_LOAD_ADDR. */
PRIVATE_FUNCTION_START(read_inode_data)
	pushad

	/* Set the new segment and start data address. */
	push	%es
	push	$(LOADER_LOAD_ADDR >> 4)
	pop	%es
	xorw	%di, %di

	/* Get total inode data size, divide it by block size and increment if
	 * there is a remainder, to get the total number of blocks we need to
	 * read. */
	movl	(inode + EXT2_INODE_OFF_SIZE), %eax
	xorl	%edx, %edx
	divl	block_size
	test	%edx, %edx
	jz	1f
	inc	%eax
1:
	/* Read each block to the destination.
	 * EAX = current block number, ECX = total blocks to read. */
	xorl	%ecx, %ecx
	xchg	%eax, %ecx
2:	push	%eax
	call	get_raw_block
	call	read_block

	/* Increment data buffer, handle overflowing into the next segment. */
	addw	block_size, %di
	jnc	3f
	push	%es
	addw	$0x1000, (%esp)
	pop	%es
3:	pop	%eax
	inc	%eax
	dec	%ecx
	jnz	2b

	pop	%es
	popad
	ret
FUNCTION_END(read_inode_data)

/** Look up a raw block number in the current inode.
 * @param %eax		Block number.
 * @return		Raw block number in %eax. */
PRIVATE_FUNCTION_START(get_raw_block)
	/* Test for extents. */
	testl	$EXT4_EXTENTS_FL, (inode + EXT2_INODE_OFF_FLAGS)
	jnz	.Lnotsupp

	cmp	$EXT2_NDIR_BLOCKS, %eax
	jae	.Lindirect

	/* Raw block number is stored in the inode. */
	shl	$2, %eax
	movl	(inode + EXT2_INODE_OFF_BLOCK)(%eax), %eax
	ret
.Lindirect:
	sub	$EXT2_NDIR_BLOCKS, %eax
	shl	$2, %eax
	cmp	block_size, %eax
	jae	.Lnotsupp

	/* Read in the indirect block. */
	push	%eax
	movl	(inode + EXT2_INODE_OFF_BLOCK + (EXT2_IND_BLOCK * 4)), %eax
	call	read_block
	pop	%eax
	push	%di
	addw	%ax, %di
	movl	%es:(%di), %eax
	pop	%di
	ret
.Lnotsupp:
	ERROR(msg_not_implemented)
FUNCTION_END(get_raw_block)

/** More error strings. */
msg_not_implemented:	.asciz "Not implemented!\r\n"

/** Pad up to 1KB. */
. = _start + 0x3FF
.byte 0

.section .bss

/** Superblock. This is read into memory along with our second half. */
superblock:		.fill EXT2_SUPERBLOCK_SIZE

/** Current inode buffer. */
inode:			.fill EXT2_INODE_SIZE

/** Temporary buffer. */
temp_buffer:		.fill 512

/** Disk address packet. */
dap_size:		.byte 0
dap_reserved1:		.byte 0
dap_count:		.byte 0
dap_reserved2:		.byte 0
dap_buffer_offset:	.word 0
dap_buffer_segment:	.word 0
dap_lba_low:		.long 0
dap_lba_high:		.long 0

/** Saved BIOS drive ID. */
bios_drive_id:		.byte 0

/** Pre-computed FS information. */
block_size:		.long 0
sectors_per_block:	.long 0
group_table_offset:	.long 0
