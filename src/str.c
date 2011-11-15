#include <io.h>

int strlen(const char *s) {
	int ret = 0;
	while(*s++) {
		ret++;
	}
	return ret;
}
