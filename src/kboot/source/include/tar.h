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
 * @brief		TAR filesystem handler.
 */

#ifndef __TAR_H
#define __TAR_H

/** Header for a TAR file. */
typedef struct tar_header {
	char name[100];		/**< Name of entry. */
	char mode[8];		/**< Mode of entry. */
	char uid[8];		/**< User ID. */
	char gid[8];		/**< Group ID. */
	char size[12];		/**< Size of entry. */
	char mtime[12];		/**< Modification time. */
	char chksum[8];		/**< Checksum. */
	char typeflag;		/**< Type flag. */
	char linkname[100];	/**< Symbolic link name. */
	char magic[6];		/**< Magic string. */
	char version[2];	/**< TAR version. */
	char uname[32];		/**< User name. */
	char gname[32];		/**< Group name. */
	char devmajor[8];	/**< Device major. */
	char devminor[8];	/**< Device minor. */
	char prefix[155];	/**< Prefix. */
} tar_header_t;

/** TAR entry types. */
#define REGTYPE		'0'	/**< Regular file (preferred code). */
#define AREGTYPE	'\0'	/**< Regular file (alternate code). */
#define LNKTYPE		'1'	/**< Hard link. */
#define SYMTYPE		'2'	/**< Symbolic link (hard if not supported). */
#define CHRTYPE		'3'	/**< Character special. */
#define BLKTYPE		'4'	/**< Block special. */
#define DIRTYPE		'5'	/**< Directory.  */
#define FIFOTYPE	'6'	/**< Named pipe.  */
#define CONTTYPE	'7'	/**< Contiguous file. */

extern void tar_mount(void *addr, size_t size);

#endif /* __TAR_H */
