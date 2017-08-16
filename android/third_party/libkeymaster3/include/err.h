#pragma once

#if !defined(__MINGW32__)
#include <err.h>
#else

#define err(retval, ...) do { \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, "Undefined error: %d\n", errno); \
    exit(retval); \
} while(0)

#endif
