#include <stdint.h>

void memset(void *p, char c, size_t len)
{
	char *s = (char *) p;
	size_t i = 0;
	for(; i < len; i++)
		s[i] = c;
}

void *memcpy(void *dest, void *src, size_t len) {
	char *s1 = (char *) src, *s2 = (char *) dest;
	while(len--) *s2++ = *s1++;
	return dest;
}
