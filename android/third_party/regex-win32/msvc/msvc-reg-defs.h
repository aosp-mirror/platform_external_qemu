#ifdef _POSIX2_RE_DUP_MAX
#define	DUPMAX	_POSIX2_RE_DUP_MAX
#else
#define	DUPMAX	255
#endif
#define	INFINITY	(DUPMAX + 1)
#define	NC		(CHAR_MAX - CHAR_MIN + 1)

#define _DIAGASSERT(e) /* nothing */
#define __UNCONST(a)    ((void *)(uintptr_t)(const void *)(a))

#define strlcpy  __regex_strlcpy
extern size_t strlcpy(char *dst, const char *src, size_t dsize);

#define DEF_WEAK(e) /* nothing */

typedef unsigned char uch;
