#ifndef _STDARG_H
#define _STDARG_H

typedef char *va_list;

#ifdef __GNUC__
#define va_start(ap, pN)	__builtin_va_start(ap, pN) // ((ap) = ((va_list) __builtin_next_arg(pN)))
#define va_end(ap)			__builtin_va_end(ap)
#define va_arg(ap, pN)		__builtin_va_arg(ap, pN)
#define va_copy(d, s)		__builtin_va_copy(d, s)
#else
#define __va_argsize(t) (((sizeof(t) + sizeof(int) - 1) / sizeof(int)) * sizeof(int))
#define va_start(ap, pN) ((ap) = ((va_list) (*pN) + __va_argsize(pN)))
#define va_end(ap) ((void) 0)
#define va_arg(ap, t) (((ap) + __va_argsize(t)), *(((t*) (void*) ((ap) - __va_argsize(t)))))
#endif

#endif

