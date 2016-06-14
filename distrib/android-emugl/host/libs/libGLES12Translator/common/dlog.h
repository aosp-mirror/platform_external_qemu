#ifndef DLOG_H
#define DLOG_H

#include <stdio.h>

#define DEBUG 1

#if DEBUG
#define APITRACE() do { fprintf(stderr, "gles12tr: %s\n", __FUNCTION__); } while(0)
#define DLOG(...) do { fprintf(stderr, "gles12tr %s: dlog: ", __FUNCTION__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } while(0)
#else
#define APITRACE()
#define DLOG(...)
#endif

#endif
