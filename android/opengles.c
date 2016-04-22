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

#include "android/crashreport/crash-handler.h"
#include "android/globals.h"
#include "android/hw-pipe-net.h"
#include "android/opengl/logger.h"
#include "android/utils/debug.h"
#include "android/utils/path.h"
#include "android/utils/bufprint.h"
#include "android/utils/dll.h"

#include "config-host.h"

#include <assert.h>
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

/* Declared in "android/globals.h" */
int  android_gles_fast_pipes = 1;

// Define the Render API function pointers.
#define FUNCTION_(ret, name, sig, params) \
        static ret (*name) sig = NULL;
LIST_RENDER_API_FUNCTIONS(FUNCTION_)
#undef FUNCTION_

// Define a function that initializes the function pointers by looking up
// the symbols from the shared library.
static int initOpenglesEmulationFuncs(ADynamicLibrary* rendererLib) {
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
    LIST_RENDER_API_FUNCTIONS(FUNCTION_)
#undef FUNCTION_

    return 0;
}

static ADynamicLibrary*  rendererLib;
static bool              rendererUsesSubWindow;
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
        derror("Could not load OpenGLES emulation library [%s]: %s",
               RENDERER_LIB_NAME, error);
        return -1;
    }

    android_opengles_pipes_init();

    /* Resolve the functions */
    if (initOpenglesEmulationFuncs(rendererLib) < 0) {
        derror("OpenGLES emulation library mismatch. Be sure to use the correct version!");
        goto BAD_EXIT;
    }

    if (!initLibrary()) {
        derror("OpenGLES initialization failed!");
        goto BAD_EXIT;
    }

    rendererUsesSubWindow = true;
    const char* env = getenv("ANDROID_GL_SOFTWARE_RENDERER");
    if (env && env[0] != '\0' && env[0] != '0') {
        rendererUsesSubWindow = false;
    }

    if (android_gles_fast_pipes) {
#ifdef _WIN32
        /* XXX: NEED Win32 pipe implementation */
        setStreamMode(RENDER_API_STREAM_MODE_TCP);
#else
        setStreamMode(RENDER_API_STREAM_MODE_UNIX);
#endif
    } else {
        setStreamMode(RENDER_API_STREAM_MODE_TCP);
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

    android_init_opengl_logger();

    emugl_logger_struct logfuncs;
    logfuncs.coarse = android_opengl_logger_write;
    logfuncs.fine = android_opengl_cxt_logger_write;

    if (!initOpenGLRenderer(width,
                            height,
                            rendererUsesSubWindow,
                            rendererAddress,
                            sizeof(rendererAddress),
                            logfuncs,
                            crashhandler_die_format)) {
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

static char* strdupBaseString(const char* src) {
    const char* begin = strchr(src, '(');
    if (!begin) {
        return strdup(src);
    }

    const char* end = strrchr(begin + 1, ')');
    if (!end) {
        return strdup(src);
    }

    // src is of the form:
    // "foo (barzzzzzzzzzz)"
    //       ^            ^
    //       (b+1)        e
    //     = 5            18
    int len;
    begin += 1;
    len = end - begin;

    char* result;
    result = (char*)malloc(len + 1);
    memcpy(result, begin, len);
    result[len] = '\0';
    return result;
}

void android_getOpenglesHardwareStrings(char** vendor,
                                        char** renderer,
                                        char** version) {
    const char *vendorSrc, *rendererSrc, *versionSrc;

    assert(vendor != NULL && renderer != NULL && version != NULL);
    assert(*vendor == NULL && *renderer == NULL && *version == NULL);
    if (!rendererStarted) {
        D("Can't get OpenGL ES hardware strings when renderer not started");
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
        *vendor = strdupBaseString(vendorSrc);
        *renderer = strdupBaseString(rendererSrc);
        *version = strdupBaseString(versionSrc);
    } else {
        *vendor = strdup(vendorSrc);
        *renderer = strdup(rendererSrc);
        *version = strdup(versionSrc);
    }
}

void
android_stopOpenglesRenderer(void)
{
    if (rendererStarted) {
        android_stop_opengl_logger();
        stopOpenGLRenderer();
        rendererStarted = 0;
    }
}

int
android_showOpenglesWindow(void* window, int wx, int wy, int ww, int wh,
                           int fbw, int fbh, float dpr, float rotation)
{
    if (!rendererStarted) {
        return -1;
    }
    FBNativeWindowType win = (FBNativeWindowType)(uintptr_t)window;
    bool success = showOpenGLSubwindow(
            win, wx, wy, ww, wh, fbw, fbh, dpr, rotation);
    return success ? 0 : -1;
}

void
android_setOpenglesTranslation(float px, float py)
{
    if (rendererStarted) {
        setOpenGLDisplayTranslation(px, py);
    }
}

int
android_hideOpenglesWindow(void)
{
    if (!rendererStarted) {
        return -1;
    }
    bool success = destroyOpenGLSubwindow();
    return success ? 0 : -1;
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
