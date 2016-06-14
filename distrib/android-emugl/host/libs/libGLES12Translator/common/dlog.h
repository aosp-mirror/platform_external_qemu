#ifndef DLOG_H
#define DLOG_H

#include "android/utils/debug.h"

#define DEBUG 0

#if DEBUG
#define APITRACE() do { \
    if (!VERBOSE_CHECK(gles1emu)) VERBOSE_ENABLE(gles1emu); \
    VERBOSE_TID_FUNCTION_DPRINT(gles1emu, "(gles1->2 translator) "); \
} while (0)
#define DLOG(...) do { \
    if (!VERBOSE_CHECK(gles1emu)) VERBOSE_ENABLE(gles1emu); \
    VERBOSE_TID_FUNCTION_DPRINT(gles1emu, "(gles1->2 translator): ", ##__VA_ARGS__); \
} while (0)
#else
#define APITRACE()
#define DLOG(...)
#endif

#endif
