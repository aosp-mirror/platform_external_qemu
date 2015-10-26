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

#include "android/opengles.h"

#include "config-host.h"
#include "qemu-common.h"
#include "qemu/shared-library.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

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

/* The list of functions that are exported by the top-level GPU emulation
 * library. |X| must be a macro that takes four arguments:
 *     - the function's return type.
 *     - the function's name.
 *     - the function signature definition.
 *     - the function parameters list during a call.
 */
#define RENDERER_FUNCTIONS_LIST(X) \
  X(int, initLibrary, (void), ()) \
  X(int, setStreamMode, (int mode), (mode)) \
  X(int, initOpenGLRenderer, (int width, int height, bool useSubWindow, char* addr, size_t addrLen), (width, height, addr, addrLen)) \
  X(void, getHardwareStrings, (const char** vendors, const char** renderer, const char** version), (vendors, renderer, version)) \
  X(void, setPostCallback, (OnPostFunc onPost, void* onPostContext), (onPost, onPostContext)) \
  X(bool, createOpenGLSubwindow, (FBNativeWindowType window, int wx, int wy, int ww, int wh, int fbw, int fbh, float zRot), (window, wx, wy, ww, wh, fbw, fbh, zRot)) \
  X(bool, destroyOpenGLSubwindow, (void), ()) \
  X(void, setOpenGLDisplayRotation, (float zRot), (zRot)) \
  X(void, repaintOpenGLDisplay, (void), ()) \
  X(int, stopOpenGLRenderer, (void), ()) \

#include <stdio.h>
#include <stdlib.h>

#define D(...)   ((void)0)
#define DD(...)  ((void)0)
#define E(...)   fprintf(stderr, __VA_ARGS__)

/* Name of the GLES rendering library we're going to use */
#if UINTPTR_MAX == UINT32_MAX
#define RENDERER_LIB_NAME  "libOpenglRender"
#elif UINTPTR_MAX == UINT64_MAX
#define RENDERER_LIB_NAME  "lib64OpenglRender"
#else
#error Unknown UINTPTR_MAX
#endif

// Define the corresponding function pointers.
#define DEFINE_FUNCTION_POINTER(ret, name, sig, params) \
        static ret (*name) sig = NULL;
RENDERER_FUNCTIONS_LIST(DEFINE_FUNCTION_POINTER)

// Define a function that initializes the function pointers by looking up
// the symbols from the shared library.
static int initOpenglesEmulationFuncs(SharedLibrary* rendererLib)
{
    void *symbol;

#define LOAD_FUNCTION_POINTER(ret, name, sig, params) \
    symbol = shared_library_find(rendererLib, #name); \
    if (symbol != NULL) { \
        name = symbol; \
    } else { \
        E("GLES emulation: Could not find required symbol: %s\n", #name); \
        return -1; \
    }
    RENDERER_FUNCTIONS_LIST(LOAD_FUNCTION_POINTER)

    return 0;
}


/* Defined in android/hw-pipe-net.c */
extern int android_init_opengles_pipes(void);

static SharedLibrary *rendererLib;
static bool rendererUsesSubWindow;
static int rendererStarted;
static char rendererAddress[256];

int android_initOpenglesEmulation(void)
{
    Error *error = NULL;

    if (rendererLib != NULL) {
        return 0;
    }
    D("Initializing hardware OpenGLES emulation support");

    rendererLib = shared_library_open(RENDERER_LIB_NAME, &error);
    if (rendererLib == NULL) {
        E("Could not load OpenGLES emulation library [%s]: %s",
               RENDERER_LIB_NAME, error_get_pretty(error));
        goto ERROR_EXIT;
    }

    // TODO(digit): Implement this!
    // android_init_opengles_pipes();

    /* Resolve the functions */
    if (initOpenglesEmulationFuncs(rendererLib) < 0) {
        E("OpenGLES emulation library mismatch. Be sure to use the correct version!");
        goto ERROR_LIB_EXIT;
    }

    if (!initLibrary()) {
        E("OpenGLES initialization failed!");
        goto ERROR_LIB_EXIT;
    }

    rendererUsesSubWindow = true;
    const char* env = getenv("ANDROID_GL_SOFTWARE_RENDERER");
    if (env && env[0] != '\0' && env[0] != '0') {
        rendererUsesSubWindow = false;
    }

#ifdef _WIN32
    /* XXX: NEED Win32 pipe implementation */
    setStreamMode(STREAM_MODE_TCP);
#else
    setStreamMode(STREAM_MODE_UNIX);
#endif
    return 0;

ERROR_LIB_EXIT:
    E("OpenGLES emulation library could not be initialized!");
    shared_library_close(rendererLib);
    rendererLib = NULL;
ERROR_EXIT:
    error_free(error);
    return -1;
}

int android_startOpenglesRenderer(int width, int height)
{
    if (!rendererLib) {
        D("Can't start OpenGLES renderer without support libraries");
        return -1;
    }

    if (rendererStarted) {
        return 0;
    }

    if (!initOpenGLRenderer(width,
                            height,
                            rendererUsesSubWindow,
                            rendererAddress,
                            sizeof(rendererAddress))) {
        D("Can't start OpenGLES renderer?");
        return -1;
    }

    rendererStarted = 1;
    return 0;
}

void android_setPostCallback(OnPostFunc onPost, void* onPostContext)
{
    if (rendererLib) {
        setPostCallback(onPost, onPostContext);
    }
}

static void extractBaseString(char* dst, const char* src, size_t dstSize)
{
    const char* begin = strchr(src, '(');
    const char* end = strrchr(src, ')');

    if (!begin || !end) {
        pstrcpy(dst, dstSize, src);
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

    pstrcpy(dst, end - begin + 1, begin);
}

void android_getOpenglesHardwareStrings(char *vendor, size_t vendorBufSize,
                                        char *renderer, size_t rendererBufSize,
                                        char *version, size_t versionBufSize)
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

    D("OpenGL Vendor=[%s]", vendorSrc);
    D("OpenGL Renderer=[%s]", rendererSrc);
    D("OpenGL Version=[%s]", versionSrc);

    /* Special case for the default ES to GL translators: extract the strings
     * of the underlying OpenGL implementation. */
    if (strncmp(vendorSrc, "Google", 6) == 0 &&
            strncmp(rendererSrc, "Android Emulator OpenGL ES Translator", 37) == 0) {
        extractBaseString(vendor, vendorSrc, vendorBufSize);
        extractBaseString(renderer, rendererSrc, rendererBufSize);
        extractBaseString(version, versionSrc, versionBufSize);
    } else {
        pstrcpy(vendor, vendorBufSize, vendorSrc);
        pstrcpy(renderer, rendererBufSize, rendererSrc);
        pstrcpy(version, versionBufSize, versionSrc);
    }
}

void android_stopOpenglesRenderer(void)
{
    if (rendererStarted) {
        stopOpenGLRenderer();
        rendererStarted = 0;
    }
}

int android_showOpenglesWindow(void* window,
                               int wx,
                               int wy,
                               int ww,
                               int wh,
                               int fbw,
                               int fbh,
                               float rotation)
{
    if (!rendererStarted) {
        return -1;
    }
    FBNativeWindowType win = (FBNativeWindowType)(uintptr_t)window;
    bool success = createOpenGLSubwindow(
            win, wx, wy, ww, wh, fbw, fbh, rotation);
    return success ? 0 : -1;
}

int android_hideOpenglesWindow(void)
{
    if (!rendererStarted) {
        return -1;
    }
    bool success = destroyOpenGLSubwindow();
    return success ? 0 : -1;
}

void android_redrawOpenglesWindow(void)
{
    if (rendererStarted) {
        repaintOpenGLDisplay();
    }
}

void android_gles_server_path(char* buff, size_t buffsize)
{
    pstrcpy(buff, buffsize, rendererAddress);
}
