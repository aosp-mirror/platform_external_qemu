/* Copyright (C) 2011 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#include "config-host.h"
#include "android/opengles.h"
#include <assert.h>

/* Declared in "android/globals.h" */
int  android_gles_fast_pipes = 1;

#include "android/globals.h"
#include <android/utils/debug.h>
#include <android/utils/path.h>
#include <android/utils/bufprint.h>
#include <android/utils/dll.h>

// NOTE: The declarations below should be equivalent to those in
// <libOpenglRender/render_api_platform_types.h>
#ifdef _WIN32
#include <windows.h>
typedef HDC FBNativeDisplayType;
typedef HWND FBNativeWindowType;
#elif defined(__linux__)
// Really a Window, which is defined as 32-bit unsigned long on all platforms
// but we don't want to include the X11 headers here.
typedef uint32_t FBNativeWindowType;
#elif defined(__APPLE__)
typedef void* FBNativeWindowType;
#else
#warning "unsupported platform"
#endif

// NOTE: The declarations below should be equivalent to those in
// <libOpenglRender/render_api.h>

/* list of constants to be passed to setStreamMode */
#define STREAM_MODE_DEFAULT   0
#define STREAM_MODE_TCP       1
#define STREAM_MODE_UNIX      2
#define STREAM_MODE_PIPE      3

#define RENDERER_FUNCTIONS_LIST \
  FUNCTION_(int, initLibrary, (void), ()) \
  FUNCTION_(int, setStreamMode, (int mode), (mode)) \
  FUNCTION_(int, initOpenGLRenderer, (int width, int height, char* addr, size_t addrLen), (width, height, addr, addrLen)) \
  FUNCTION_VOID_(getHardwareStrings, (const char** vendors, const char** renderer, const char** version), (vendors, renderer, version)) \
  FUNCTION_VOID_(setPostCallback, (OnPostFunc onPost, void* onPostContext), (onPost, onPostContext)) \
  FUNCTION_(int, createOpenGLSubwindow, (FBNativeWindowType window, int x, int y, int width, int height, float zRot), (window, x, y, width, height, zRot)) \
  FUNCTION_(int, destroyOpenGLSubwindow, (void), ()) \
  FUNCTION_VOID_(setOpenGLDisplayRotation, (float zRot), (zRot)) \
  FUNCTION_VOID_(repaintOpenGLDisplay, (void), ()) \
  FUNCTION_(int, stopOpenGLRenderer, (void), ()) \

#include <stdio.h>
#include <stdlib.h>

#define D(...)  VERBOSE_PRINT(init,__VA_ARGS__)
#define DD(...) VERBOSE_PRINT(gles,__VA_ARGS__)

/* Name of the GLES rendering library we're going to use */
#if UINTPTR_MAX == UINT32_MAX
#define RENDERER_LIB_NAME  "libOpenglRender"
#elif UINTPTR_MAX == UINT64_MAX
#define RENDERER_LIB_NAME  "lib64OpenglRender"
#else
#error Unknown UINTPTR_MAX
#endif

// Define the corresponding function pointers.
#define FUNCTION_(ret, name, sig, params) \
        static ret (*name) sig = NULL;
#define FUNCTION_VOID_(name, sig, params) \
        static void (*name) sig = NULL;
RENDERER_FUNCTIONS_LIST
#undef FUNCTION_
#undef FUNCTION_VOID_

// Define a function that initializes the function pointers by looking up
// the symbols from the shared library.
static int
initOpenglesEmulationFuncs(ADynamicLibrary* rendererLib)
{
    void*  symbol;
    char*  error;

#define FUNCTION_(ret, name, sig, params) \
    symbol = adynamicLibrary_findSymbol(rendererLib, #name, &error); \
    if (symbol != NULL) { \
        name = symbol; \
    } else { \
        derror("GLES emulation: Could not find required symbol (%s): %s", #name, error); \
        free(error); \
        return -1; \
    }
#define FUNCTION_VOID_(name, sig, params) FUNCTION_(void, name, sig, params)
RENDERER_FUNCTIONS_LIST
#undef FUNCTION_VOID_
#undef FUNCTION_

    return 0;
}


/* Defined in android/hw-pipe-net.c */
extern int android_init_opengles_pipes(void);

static ADynamicLibrary*  rendererLib;
static int               rendererStarted;
static char              rendererAddress[256];

int
android_initOpenglesEmulation(void)
{
    char* error = NULL;

    if (rendererLib != NULL)
        return 0;

    D("Initializing hardware OpenGLES emulation support");

    rendererLib = adynamicLibrary_open(RENDERER_LIB_NAME, &error);
    if (rendererLib == NULL) {
        derror("Could not load OpenGLES emulation library: %s", error);
        return -1;
    }

    android_init_opengles_pipes();

    /* Resolve the functions */
    if (initOpenglesEmulationFuncs(rendererLib) < 0) {
        derror("OpenGLES emulation library mismatch. Be sure to use the correct version!");
        goto BAD_EXIT;
    }

    if (!initLibrary()) {
        derror("OpenGLES initialization failed!");
        goto BAD_EXIT;
    }

    if (android_gles_fast_pipes) {
#ifdef _WIN32
        /* XXX: NEED Win32 pipe implementation */
        setStreamMode(STREAM_MODE_TCP);
#else
	    setStreamMode(STREAM_MODE_UNIX);
#endif
    } else {
	    setStreamMode(STREAM_MODE_TCP);
    }
    return 0;

BAD_EXIT:
    derror("OpenGLES emulation library could not be initialized!");
    adynamicLibrary_close(rendererLib);
    rendererLib = NULL;
    return -1;
}

int
android_startOpenglesRenderer(int width, int height)
{
    if (!rendererLib) {
        D("Can't start OpenGLES renderer without support libraries");
        return -1;
    }

    if (rendererStarted) {
        return 0;
    }

    if (!initOpenGLRenderer(width, height, rendererAddress, sizeof(rendererAddress))) {
        D("Can't start OpenGLES renderer?");
        return -1;
    }

    rendererStarted = 1;
    return 0;
}

void
android_setPostCallback(OnPostFunc onPost, void* onPostContext)
{
    if (rendererLib) {
        setPostCallback(onPost, onPostContext);
    }
}

static void strncpy_safe(char* dst, const char* src, size_t n)
{
    strncpy(dst, src, n);
    dst[n-1] = '\0';
}

static void extractBaseString(char* dst, const char* src, size_t dstSize)
{
    const char* begin = strchr(src, '(');
    const char* end = strrchr(src, ')');

    if (!begin || !end) {
        strncpy_safe(dst, src, dstSize);
        return;
    }
    begin += 1;

    // "foo (bar)"
    //       ^  ^
    //       b  e
    //     = 5  8
    // substring with NUL-terminator is end-begin+1 bytes
    if (end - begin + 1 > dstSize) {
        end = begin + dstSize - 1;
    }

    strncpy_safe(dst, begin, end - begin + 1);
}

void
android_getOpenglesHardwareStrings(char* vendor, size_t vendorBufSize,
                                   char* renderer, size_t rendererBufSize,
                                   char* version, size_t versionBufSize)
{
    const char *vendorSrc, *rendererSrc, *versionSrc;

    assert(vendorBufSize > 0 && rendererBufSize > 0 && versionBufSize > 0);
    assert(vendor != NULL && renderer != NULL && version != NULL);

    if (!rendererStarted) {
        D("Can't get OpenGL ES hardware strings when renderer not started");
        vendor[0] = renderer[0] = version[0] = '\0';
        return;
    }

    getHardwareStrings(&vendorSrc, &rendererSrc, &versionSrc);
    if (!vendorSrc) vendorSrc = "";
    if (!rendererSrc) rendererSrc = "";
    if (!versionSrc) versionSrc = "";

    /* Special case for the default ES to GL translators: extract the strings
     * of the underlying OpenGL implementation. */
    if (strncmp(vendorSrc, "Google", 6) == 0 &&
            strncmp(rendererSrc, "Android Emulator OpenGL ES Translator", 37) == 0) {
        extractBaseString(vendor, vendorSrc, vendorBufSize);
        extractBaseString(renderer, rendererSrc, rendererBufSize);
        extractBaseString(version, versionSrc, versionBufSize);
    } else {
        strncpy_safe(vendor, vendorSrc, vendorBufSize);
        strncpy_safe(renderer, rendererSrc, rendererBufSize);
        strncpy_safe(version, versionSrc, versionBufSize);
    }
}

void
android_stopOpenglesRenderer(void)
{
    if (rendererStarted) {
        stopOpenGLRenderer();
        rendererStarted = 0;
    }
}

int
android_showOpenglesWindow(void* window, int x, int y, int width, int height, float rotation)
{
    if (rendererStarted) {
        int success = createOpenGLSubwindow((FBNativeWindowType)(uintptr_t)window, x, y, width, height, rotation);
        return success ? 0 : -1;
    } else {
        return -1;
    }
}

int
android_hideOpenglesWindow(void)
{
    if (rendererStarted) {
        int success = destroyOpenGLSubwindow();
        return success ? 0 : -1;
    } else {
        return -1;
    }
}

void
android_redrawOpenglesWindow(void)
{
    if (rendererStarted) {
        repaintOpenGLDisplay();
    }
}

void
android_gles_server_path(char* buff, size_t buffsize)
{
    strncpy_safe(buff, rendererAddress, buffsize);
}
