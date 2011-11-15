
#ifndef _ASSERT_H
#define _ASSERT_H

#include <panic.h>

#define STR(x) # x

#define _assert(b, f, l) do { \
	if(!(b)) { \
		panic("Assertion failed in " f ":" STR(l) "."); \
	} \
} while(0)

#define assert(b) _assert(b, __FILE__, __LINE__)

#endif
