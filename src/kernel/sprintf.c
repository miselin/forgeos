#include <io.h>
#include <stdarg.h>

extern int vsprintf(char *buf, const char *fmt, va_list args);

int sprintf(char *s, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	return vsprintf(s, fmt, args);
}
