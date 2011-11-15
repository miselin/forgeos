#include <io.h>
#include <string.h>

void puts(const char *s) {
	int len = strlen(s);
	while(len--) putc(*s++);
}
