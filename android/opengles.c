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

#define RENDER_API_NO_PROTOTYPES 1

#include "config-host.h"
#include "android/opengles.h"
#include "android/globals.h"
#include <android/utils/debug.h>
#include <android/utils/path.h>
#include <android/utils/bufprint.h>
#include <android/utils/dll.h>
#include <libOpenglRender/render_api.h>
#include <stdio.h>
#include <stdlib.h>

#define D(...)  VERBOSE_PRINT(init,__VA_ARGS__)
#define DD(...) VERBOSE_PRINT(gles,__VA_ARGS__)

/* Declared in "android/globals.h" */
int  android_gles_fast_pipes = 1;

/* Name of the GLES rendering library we're going to use */
#if HOST_LONG_BITS == 32
#define RENDERER_LIB_NAME  "libOpenglRender"
#elif HOST_LONG_BITS == 64
#define RENDERER_LIB_NAME  "lib64OpenglRender"
#else
#error Unknown HOST_LONG_BITS
#endif

#define DYNLINK_FUNCTIONS  \
  DYNLINK_FUNC(initLibrary) \
  DYNLINK_FUNC(setStreamMode) \
  DYNLINK_FUNC(initOpenGLRenderer) \
  DYNLINK_FUNC(createOpenGLSubwindow) \
  DYNLINK_FUNC(destroyOpenGLSubwindow) \
  DYNLINK_FUNC(repaintOpenGLDisplay) \
  DYNLINK_FUNC(stopOpenGLRenderer)

#ifndef CONFIG_STANDALONE_UI
/* Defined in android/hw-pipe-net.c */
extern int android_init_opengles_pipes(void);
#endif

static ADynamicLibrary*  rendererLib;

/* Define the function pointers */
#define DYNLINK_FUNC(name) \
    static name##Fn name = NULL;
DYNLINK_FUNCTIONS
#undef DYNLINK_FUNC

static int
initOpenglesEmulationFuncs(ADynamicLibrary* rendererLib)
{
    void*  symbol;
    char*  error;

#define DYNLINK_FUNC(name) \
    symbol = adynamicLibrary_findSymbol(rendererLib, #name, &error); \
    if (symbol != NULL) { \
        name = symbol; \
    } else { \
        derror("GLES emulation: Could not find required symbol (%s): %s", #name, error); \
        free(error); \
        return -1; \
    }
DYNLINK_FUNCTIONS
#undef DYNLINK_FUNC

    return 0;
}

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

#ifndef CONFIG_STANDALONE_UI
    android_init_opengles_pipes();
#endif


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
android_startOpenglesRenderer(int width, int height, OnPostFn onPost, void* onPostContext)
{
    if (!rendererLib) {
        D("Can't start OpenGLES renderer without support libraries");
        return -1;
    }

    if (!initOpenGLRenderer(width, height, ANDROID_OPENGLES_BASE_PORT, onPost, onPostContext)) {
        D("Can't start OpenGLES renderer?");
        return -1;
    }
    return 0;
}

void
android_stopOpenglesRenderer(void)
{
    if (rendererLib) {
        stopOpenGLRenderer();
    }
}

int
android_showOpenglesWindow(void* window, int x, int y, int width, int height, float rotation)
{
    if (rendererLib) {
        int success = createOpenGLSubwindow((FBNativeWindowType)window, x, y, width, height, rotation);
        return success ? 0 : -1;
    } else {
        return -1;
    }
}

int
android_hideOpenglesWindow(void)
{
    if (rendererLib) {
        int success = destroyOpenGLSubwindow();
        return success ? 0 : -1;
    } else {
        return -1;
    }
}

void
android_redrawOpenglesWindow(void)
{
    if (rendererLib) {
        repaintOpenGLDisplay();
    }
}

void
android_gles_unix_path(char* buff, size_t buffsize, int port)
{
    const char* user = getenv("USER");
    char *p = buff, *end = buff + buffsize;

    /* The logic here must correspond to the one inside
     * development/tools/emulator/opengl/shared/libOpenglCodecCommon/UnixStream.cpp */
    p = bufprint(p, end, "/tmp/");
    if (user && user[0]) {
        p = bufprint(p, end, "android-%s/", user);
    }
    p = bufprint(p, end, "qemu-gles-%d", port);
}
