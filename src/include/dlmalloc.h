
#ifndef _DLMALLOC_H
#define _DLMALLOC_H

#include <stdint.h>
#include <util.h>

#define ABORT					dlmalloc_abort
#define MORECORE				dlmalloc_sbrk
#define HAVE_MORECORE			1
#define MORECORE_CANNOT_TRIM	1
#define MORECORE_CONTIGUOUS		1
#define HAVE_MMAP				0
#define MALLOC_FAILURE_ACTION

#define NO_MALLOC_STATS			1

#define LACKS_TIME_H			1
#define LACKS_SYS_MMAN_H		1
#define LACKS_UNISTD_H			1
#define LACKS_FCNTL_H			1
#define LACKS_SYS_PARAM_H		1
#define LACKS_STRINGS_H			1
#define LACKS_STRING_H			1
#define LACKS_SYS_TYPES_H
#define LACKS_STDLIB_H
#define LACKS_ERRNO_H

extern void dlmalloc_abort();
extern void *dlmalloc_sbrk(size_t incr);

#define EINVAL					-1000
#define ENOMEM					-1001

#endif
