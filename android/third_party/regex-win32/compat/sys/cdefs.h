#pragma once

#include <stddef.h>

#define __predict_false(exp)    __builtin_expect((exp) != 0, 0)
#define __RCSID(e) /* nothing */

#define reallocarr  __regex_reallocarr
extern int reallocarr(void *ptr, size_t number, size_t size);

#define strlcpy  __regex_strlcpy
extern size_t strlcpy(char *dst, const char *src, size_t dsize);

#ifdef _MSC_VER
#include "msvc-posix.h"
#else
#include_next <sys/cdefs.h>
#endif
