#ifndef _STDINT_H
#define _STDINT_H

typedef signed char		int8_t;
typedef signed short	int16_t;
typedef signed long		int32_t;
typedef signed int		int64_t __attribute__((mode (__DI__)));

typedef unsigned char	uint8_t;
typedef unsigned short	uint16_t;
typedef unsigned long	uint32_t;
typedef unsigned int	uint64_t __attribute__((mode (__DI__)));

typedef unsigned long	size_t;

typedef signed long		intptr_t;
typedef unsigned long	uintptr_t;

typedef uintptr_t		paddr_t;
typedef uintptr_t		vaddr_t;

typedef int64_t			intmax_t;
typedef uint64_t		uintmax_t;

#endif
