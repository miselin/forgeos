#include <io.h>
#include <stdarg.h>

extern int vsprintf(char *buf, const char *fmt, va_list args);

int kprintf(const char *fmt, ...) {
	int len = 0;
	char buf[512];
	va_list args;
	va_start(args, fmt);
	len = vsprintf(buf, fmt, args);
	puts(buf);
	return len;
}
