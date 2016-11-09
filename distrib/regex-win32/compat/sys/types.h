#pragma once

#include <stdint.h>
typedef uint32_t u_int32_t;

#define _DIAGASSERT(e) /* nothing */
#define _POSIX2_RE_DUP_MAX 256

#define __UNCONST(a)    ((void *)(uintptr_t)(const void *)(a))

#define strlcpy  __regex_strlcpy
extern size_t strlcpy(char *dst, const char *src, size_t dsize);

#define DEF_WEAK(e) /* nothing */

#include_next <sys/types.h>
