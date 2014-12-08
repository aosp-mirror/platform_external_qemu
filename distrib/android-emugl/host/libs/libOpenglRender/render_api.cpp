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
#include "libOpenglRender/render_api.h"
#include "IOStream.h"
#include "FrameBuffer.h"
#include "RenderServer.h"
#include "TimeUtils.h"

#include "TcpStream.h"
#ifdef _WIN32
#include "Win32PipeStream.h"
#else
#include "UnixStream.h"
#endif

#include "EGLDispatch.h"
#include "GLDispatch.h"
#include "GL2Dispatch.h"

static RenderServer *s_renderThread = NULL;
static char s_renderAddr[256];

static IOStream *createRenderThread(int p_stream_buffer_size,
                                    unsigned int clientFlags);

int initLibrary(void)
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
    if (!init_gl_dispatch()) {
        // Failed to load GLES
        ERR("Failed to init_gl_dispatch\n");
        return false;
    }

    /* failure to init the GLES2 dispatch table is not fatal */
    init_gl2_dispatch();

    return true;
}

int initOpenGLRenderer(int width, int height, char* addr, size_t addrLen)
{

    //
    // Fail if renderer is already initialized
    //
    if (s_renderThread) {
        return false;
    }

    //
    // initialize the renderer and listen to connections
    // on a thread in the current process.
    //
    bool inited = FrameBuffer::initialize(width, height);
    if (!inited) {
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

void setPostCallback(OnPostFn onPost, void* onPostContext)
{
    FrameBuffer* fb = FrameBuffer::getFB();
    if (fb) {
        fb->setPostCallback(onPost, onPostContext);
    }
}

void getHardwareStrings(const char** vendor, const char** renderer, const char** version)
{
    FrameBuffer* fb = FrameBuffer::getFB();
    if (fb) {
        fb->getGLStrings(vendor, renderer, version);
    } else {
        *vendor = *renderer = *version = NULL;
    }
}

int stopOpenGLRenderer(void)
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

    return ret;
}

int createOpenGLSubwindow(FBNativeWindowType window,
                           int x, int y, int width, int height, float zRot)
{
    if (s_renderThread) {
        return FrameBuffer::setupSubWindow(window,x,y,width,height, zRot);
    }
    else {
        //
        // XXX: should be implemented by sending the renderer process
        //      a request
        ERR("%s not implemented for separate renderer process !!!\n",
            __FUNCTION__);
    }
    return false;
}

int destroyOpenGLSubwindow(void)
{
    if (s_renderThread) {
        return FrameBuffer::removeSubWindow();
    }
    else {
        //
        // XXX: should be implemented by sending the renderer process
        //      a request
        ERR("%s not implemented for separate renderer process !!!\n",
                __FUNCTION__);
        return false;
    }
}

void setOpenGLDisplayRotation(float zRot)
{
    if (s_renderThread) {
        FrameBuffer *fb = FrameBuffer::getFB();
        if (fb) {
            fb->setDisplayRotation(zRot);
        }
    }
    else {
        //
        // XXX: should be implemented by sending the renderer process
        //      a request
        ERR("%s not implemented for separate renderer process !!!\n",
                __FUNCTION__);
    }
}

void repaintOpenGLDisplay(void)
{
    if (s_renderThread) {
        FrameBuffer *fb = FrameBuffer::getFB();
        if (fb) {
            fb->repost();
        }
    }
    else {
        //
        // XXX: should be implemented by sending the renderer process
        //      a request
        ERR("%s not implemented for separate renderer process !!!\n",
                __FUNCTION__);
    }
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

int
setStreamMode(int mode)
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
