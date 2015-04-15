/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include "render_api.h"

#include "IOStream.h"
#include "RenderServer.h"
#include "RenderWindow.h"
#include "TimeUtils.h"

#include "TcpStream.h"
#ifdef _WIN32
#include "Win32PipeStream.h"
#else
#include "UnixStream.h"
#endif

#include "EGLDispatch.h"
#include "GLESv1Dispatch.h"
#include "GLESv2Dispatch.h"

#include <string.h>

static RenderServer* s_renderThread = NULL;
static char s_renderAddr[256];

static RenderWindow* s_renderWindow = NULL;

static IOStream *createRenderThread(int p_stream_buffer_size,
                                    unsigned int clientFlags);

RENDER_APICALL int RENDER_APIENTRY initLibrary(void)
{
    //
    // Load EGL Plugin
    //
    if (!init_egl_dispatch()) {
        // Failed to load EGL
        printf("Failed to init_egl_dispatch\n");
        return false;
    }

    //
    // Load GLES Plugin
    //
    if (!init_gles1_dispatch()) {
        // Failed to load GLES
        ERR("Failed to init_gles1_dispatch\n");
        return false;
    }

    /* failure to init the GLES2 dispatch table is not fatal */
    if (!init_gles2_dispatch()) {
        ERR("Failed to init_gles2_dispatch\n");
        return false;
    }

    return true;
}

RENDER_APICALL int RENDER_APIENTRY initOpenGLRenderer(
        int width, int height, char* addr, size_t addrLen) {
    //
    // Fail if renderer is already initialized
    //
    if (s_renderThread) {
        return false;
    }

    // kUseThread is used to determine whether the RenderWindow should use
    // a separate thread to manage its subwindow GL/GLES context.
    // Experience shows that:
    //
    // - It is necessary on Linux/XGL and OSX/Cocoa to avoid corruption
    //   issues with the GL state of the main window, resulting in garbage
    //   or black content of the non-framebuffer UI parts.
    //
    // - It must be disabled on Windows, otherwise the main window becomes
    //   unresponsive after a few seconds of user interaction (e.g. trying to
    //   move it over the desktop). Probably due to the subtle issues around
    //   input on this platform (input-queue is global, message-queue is
    //   per-thread). Also, this messes considerably the display of the
    //   main window when running the executable under Wine.
    //
#ifdef _WIN32
    bool kUseThread = false;
#else
    bool kUseThread = true;
#endif

    //
    // initialize the renderer and listen to connections
    // on a thread in the current process.
    //
    s_renderWindow = new RenderWindow(width, height, kUseThread);
    if (!s_renderWindow) {
        ERR("Could not create rendering window class");
        return false;
    }
    if (!s_renderWindow->isValid()) {
        ERR("Could not initialize emulated framebuffer");
        delete s_renderWindow;
        return false;
    }

    s_renderThread = RenderServer::create(addr, addrLen);
    if (!s_renderThread) {
        return false;
    }
    strncpy(s_renderAddr, addr, sizeof(s_renderAddr));

    s_renderThread->start();

    return true;
}

RENDER_APICALL void RENDER_APIENTRY setPostCallback(
        OnPostFn onPost, void* onPostContext) {
    if (s_renderWindow) {
        s_renderWindow->setPostCallback(onPost, onPostContext);
    } else {
        ERR("Calling setPostCallback() before creating render window!");
    }
}

RENDER_APICALL void RENDER_APIENTRY getHardwareStrings(
        const char** vendor,
        const char** renderer,
        const char** version) {
    if (s_renderWindow &&
        s_renderWindow->getHardwareStrings(vendor, renderer, version)) {
        return;
    }
    *vendor = *renderer = *version = NULL;
}

RENDER_APICALL int RENDER_APIENTRY stopOpenGLRenderer(void)
{
    bool ret = false;

    // open a dummy connection to the renderer to make it
    // realize the exit request.
    // (send the exit request in clientFlags)
    IOStream *dummy = createRenderThread(8, IOSTREAM_CLIENT_EXIT_SERVER);
    if (!dummy) return false;

    if (s_renderThread) {
        // wait for the thread to exit
        ret = s_renderThread->wait(NULL);

        delete s_renderThread;
        s_renderThread = NULL;
    }

    if (s_renderWindow) {
        delete s_renderWindow;
        s_renderWindow = NULL;
    }

    return ret;
}

RENDER_APICALL bool RENDER_APIENTRY createOpenGLSubwindow(
        FBNativeWindowType window_id,
        int x,
        int y,
        int width,
        int height,
        float zRot)
{
    RenderWindow* window = s_renderWindow;

    if (window) {
       return window->setupSubWindow(window_id,x,y,width,height, zRot);
    }
    // XXX: should be implemented by sending the renderer process
    //      a request
    ERR("%s not implemented for separate renderer process !!!\n",
        __FUNCTION__);
    return false;
}

RENDER_APICALL bool RENDER_APIENTRY destroyOpenGLSubwindow(void)
{
    RenderWindow* window = s_renderWindow;

    if (window) {
        return window->removeSubWindow();
    }

    // XXX: should be implemented by sending the renderer process
    //      a request
    ERR("%s not implemented for separate renderer process !!!\n",
            __FUNCTION__);
    return false;
}

RENDER_APICALL void RENDER_APIENTRY setOpenGLDisplayRotation(float zRot)
{
    RenderWindow* window = s_renderWindow;

    if (window) {
        window->setRotation(zRot);
        return;
    }
    // XXX: should be implemented by sending the renderer process
    //      a request
    ERR("%s not implemented for separate renderer process !!!\n",
            __FUNCTION__);
}

RENDER_APICALL void RENDER_APIENTRY repaintOpenGLDisplay(void)
{
    RenderWindow* window = s_renderWindow;

    if (window) {
        window->repaint();
        return;
    }
    // XXX: should be implemented by sending the renderer process
    //      a request
    ERR("%s not implemented for separate renderer process !!!\n",
            __FUNCTION__);
}


/* NOTE: For now, always use TCP mode by default, until the emulator
 *        has been updated to support Unix and Win32 pipes
 */
#define  DEFAULT_STREAM_MODE  STREAM_MODE_TCP

int gRendererStreamMode = DEFAULT_STREAM_MODE;

IOStream *createRenderThread(int p_stream_buffer_size, unsigned int clientFlags)
{
    SocketStream*  stream = NULL;

    if (gRendererStreamMode == STREAM_MODE_TCP) {
        stream = new TcpStream(p_stream_buffer_size);
    } else {
#ifdef _WIN32
        stream = new Win32PipeStream(p_stream_buffer_size);
#else /* !_WIN32 */
        stream = new UnixStream(p_stream_buffer_size);
#endif
    }

    if (!stream) {
        ERR("createRenderThread failed to create stream\n");
        return NULL;
    }
    if (stream->connect(s_renderAddr) < 0) {
        ERR("createRenderThread failed to connect\n");
        delete stream;
        return NULL;
    }

    //
    // send clientFlags to the renderer
    //
    unsigned int *pClientFlags =
                (unsigned int *)stream->allocBuffer(sizeof(unsigned int));
    *pClientFlags = clientFlags;
    stream->commitBuffer(sizeof(unsigned int));

    return stream;
}

RENDER_APICALL int RENDER_APIENTRY setStreamMode(int mode)
{
    switch (mode) {
        case STREAM_MODE_DEFAULT:
            mode = DEFAULT_STREAM_MODE;
            break;

        case STREAM_MODE_TCP:
            break;

#ifndef _WIN32
        case STREAM_MODE_UNIX:
            break;
#else /* _WIN32 */
        case STREAM_MODE_PIPE:
            break;
#endif /* _WIN32 */
        default:
            // Invalid stream mode
            return false;
    }
    gRendererStreamMode = mode;
    return true;
}
