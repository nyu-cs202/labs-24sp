#pragma once

#include "panic.h"

#define _assert(lineno, filename, stmt) \
{ \
if (!(stmt)) \
	_panic(lineno, filename, "\x1B[31massert failed\x1B[m: %s", #stmt); \
}

#define assert(stmt) _assert(__LINE__, __FILE__, stmt)
