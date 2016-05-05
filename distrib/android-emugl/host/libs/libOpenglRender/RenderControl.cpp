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

#include "RenderControl.h"

#include "DispatchTables.h"
#include "FbConfig.h"
#include "FenceSyncInfo.h"
#include "FrameBuffer.h"
#include "RenderContext.h"
#include "RenderThreadInfo.h"
#include "ChecksumCalculatorThreadInfo.h"
#include "OpenGLESDispatch/EGLDispatch.h"
#include "emugl/common/feature_control.h"

#include <inttypes.h>

using android::base::Lock;
using android::base::AutoLock;

#define DEBUG 0

#if DEBUG && !defined(_WIN32)
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#define DPRINT(...) do { fprintf(stderr, "tid=0x%lx | ", syscall(__NR_gettid)); fprintf(stderr, __VA_ARGS__); } while(0)
#else
#define DPRINT(...)
#endif

static const GLint rendererVersion = 1;
static const char* kAsyncSwapStr = "ANDROID_EMU_ASYNC_SWAP";

static GLint rcGetRendererVersion()
{
    return rendererVersion;
}

static EGLint rcGetEGLVersion(EGLint* major, EGLint* minor)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return EGL_FALSE;
    }
    *major = (EGLint)fb->getCaps().eglMajor;
    *minor = (EGLint)fb->getCaps().eglMinor;

    return EGL_TRUE;
}

static EGLint rcQueryEGLString(EGLenum name, void* buffer, EGLint bufferSize)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return 0;
    }

    const char *str = s_egl.eglQueryString(fb->getDisplay(), name);
    if (!str) {
        return 0;
    }

    int len = strlen(str) + 1;
    if (!buffer || len > bufferSize) {
        return -len;
    }

    strcpy((char *)buffer, str);
    return len;
}

static RenderContext* create_trivial_context() {
    FrameBuffer *fb = FrameBuffer::getFB();
    const FbConfig* cfg = fb->getConfigs()->get(0);
    RenderContext* cxt = RenderContext::create(fb->getDisplay(), cfg->getEglConfig(), EGL_NO_CONTEXT, 1);
    static const EGLint pbuf_attribs[] = {
        EGL_WIDTH, 1,
        EGL_HEIGHT, 1,
        EGL_NONE
    };
    EGLSurface surf = s_egl.eglCreatePbufferSurface(fb->getDisplay(), cfg->getEglConfig(), pbuf_attribs);
    s_egl.eglMakeCurrent(fb->getDisplay(), surf, surf, cxt->getEGLContext());
    return cxt;
}

static EGLint rcGetGLString(EGLenum name, void* buffer, EGLint bufferSize)
{
    RenderThreadInfo *tInfo = RenderThreadInfo::get();
    const char *str = NULL;
    int len = 0;
    if (tInfo && tInfo->currContext.get()) {
        if (tInfo->currContext->isGL2()) {
            str = (const char *)s_gles2.glGetString(name);
        }
        else {
            str = (const char *)s_gles1.glGetString(name);
        }
        if (str) {
            len = strlen(str) + 1;
        }
    } else {
        // No GL context, create one and associate it just for getting OpenGL strings.
        tInfo->currContext = RenderContextPtr(create_trivial_context());
        str = (const char *)s_gles2.glGetString(name);
        len = strlen(str) + 1;
    }

    // We add the maximum supported GL protocol number into GL_EXTENSIONS
    bool isChecksumEnabled = emugl_feature_is_enabled(android::featurecontrol::GLPipeChecksum);
    bool asyncSwapEnabled = emugl_feature_is_enabled(android::featurecontrol::GLAsyncSwap);
    std::string glProtocolStr;

    if (isChecksumEnabled && name == GL_EXTENSIONS) {
        glProtocolStr += ChecksumCalculatorThreadInfo::getMaxVersionString();
        glProtocolStr += " ";
    }

    if (asyncSwapEnabled && name == GL_EXTENSIONS) {
        glProtocolStr += kAsyncSwapStr;
        glProtocolStr += " ";
    }

    len += glProtocolStr.size();

    if (!buffer || len > bufferSize) {
        return -len;
    }

    if (name == GL_EXTENSIONS) {
        snprintf((char *)buffer, bufferSize, "%s%s ", str ? str : "", glProtocolStr.c_str());
    } else if (str) {
        strcpy((char *)buffer, str);
    } else {
        if (bufferSize >= 1) {
            ((char*)buffer)[0] = '\0';
        }
        len = 0;
    }
    return len;
}

static EGLint rcGetNumConfigs(uint32_t* p_numAttribs)
{
    int numConfigs = 0, numAttribs = 0;

    FrameBuffer::getFB()->getConfigs()->getPackInfo(&numConfigs, &numAttribs);
    if (p_numAttribs) {
        *p_numAttribs = static_cast<uint32_t>(numAttribs);
    }
    return numConfigs;
}

static EGLint rcGetConfigs(uint32_t bufSize, GLuint* buffer)
{
    GLuint bufferSize = (GLuint)bufSize;
    return FrameBuffer::getFB()->getConfigs()->packConfigs(bufferSize, buffer);
}

static EGLint rcChooseConfig(EGLint *attribs,
                             uint32_t attribs_size,
                             uint32_t *configs,
                             uint32_t configs_size)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb || attribs_size==0) {
        return 0;
    }

    return fb->getConfigs()->chooseConfig(
            attribs, (EGLint*)configs, (EGLint)configs_size);
}

static EGLint rcGetFBParam(EGLint param)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return 0;
    }

    EGLint ret = 0;

    switch(param) {
        case FB_WIDTH:
            ret = fb->getWidth();
            break;
        case FB_HEIGHT:
            ret = fb->getHeight();
            break;
        case FB_XDPI:
            ret = 72; // XXX: should be implemented
            break;
        case FB_YDPI:
            ret = 72; // XXX: should be implemented
            break;
        case FB_FPS:
            ret = 60;
            break;
        case FB_MIN_SWAP_INTERVAL:
            ret = 1; // XXX: should be implemented
            break;
        case FB_MAX_SWAP_INTERVAL:
            ret = 1; // XXX: should be implemented
            break;
        default:
            break;
    }

    return ret;
}

static uint32_t rcCreateContext(uint32_t config,
                                uint32_t share, uint32_t glVersion)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return 0;
    }

    // To make it consistent with the guest, create GLES2 context when GL
    // version==2 or 3
    HandleType ret = fb->createRenderContext(config, share, glVersion == 2 || glVersion == 3);
    return ret;
}

static void rcDestroyContext(uint32_t context)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return;
    }

    fb->DestroyRenderContext(context);
}

static uint32_t rcCreateWindowSurface(uint32_t config,
                                      uint32_t width, uint32_t height)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return 0;
    }

    return fb->createWindowSurface(config, width, height);
}

static void rcDestroyWindowSurface(uint32_t windowSurface)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return;
    }

    fb->DestroyWindowSurface( windowSurface );
}

static uint32_t rcCreateColorBuffer(uint32_t width,
                                    uint32_t height, GLenum internalFormat)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return 0;
    }

    return fb->createColorBuffer(width, height, internalFormat);
}

static int rcOpenColorBuffer2(uint32_t colorbuffer)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return -1;
    }
    return fb->openColorBuffer( colorbuffer );
}

// Deprecated, kept for compatibility with old system images only.
// Use rcOpenColorBuffer2 instead.
static void rcOpenColorBuffer(uint32_t colorbuffer)
{
    (void) rcOpenColorBuffer2(colorbuffer);
}

static void rcCloseColorBuffer(uint32_t colorbuffer)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return;
    }
    fb->closeColorBuffer( colorbuffer );
}

static void rcFlushWindowColorBuffer(uint32_t windowSurface)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return;
    }
    if (!fb->flushWindowSurfaceColorBuffer(windowSurface)) {
        return;
    }
    return;
}

static void rcSetWindowColorBuffer(uint32_t windowSurface,
                                   uint32_t colorBuffer)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return;
    }
    fb->setWindowSurfaceColorBuffer(windowSurface, colorBuffer);
}

static EGLint rcMakeCurrent(uint32_t context,
                            uint32_t drawSurf, uint32_t readSurf)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return EGL_FALSE;
    }

    bool ret = fb->bindContext(context, drawSurf, readSurf);

    return (ret ? EGL_TRUE : EGL_FALSE);
}

static void rcFBPost(uint32_t colorBuffer)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return;
    }

    fb->post(colorBuffer);
}

static void rcFBSetSwapInterval(EGLint interval)
{
   // XXX: TBD - should be implemented
}

static void rcBindTexture(uint32_t colorBuffer)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return;
    }

    fb->bindColorBufferToTexture(colorBuffer);
}

static void rcBindRenderbuffer(uint32_t colorBuffer)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return;
    }

    fb->bindColorBufferToRenderbuffer(colorBuffer);
}

static EGLint rcColorBufferCacheFlush(uint32_t colorBuffer,
                                      EGLint postCount, int forRead)
{
   // XXX: TBD - should be implemented
   return 0;
}

static void rcReadColorBuffer(uint32_t colorBuffer,
                              GLint x, GLint y,
                              GLint width, GLint height,
                              GLenum format, GLenum type, void* pixels)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return;
    }

    fb->readColorBuffer(colorBuffer, x, y, width, height, format, type, pixels);
}

static int rcUpdateColorBuffer(uint32_t colorBuffer,
                                GLint x, GLint y,
                                GLint width, GLint height,
                                GLenum format, GLenum type, void* pixels)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return -1;
    }

    fb->updateColorBuffer(colorBuffer, x, y, width, height, format, type, pixels);
    return 0;
}

static uint32_t rcCreateClientImage(uint32_t context, EGLenum target, GLuint buffer)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return 0;
    }

    return fb->createClientImage(context, target, buffer);
}

static int rcDestroyClientImage(uint32_t image)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return 0;
    }

    return fb->destroyClientImage(image);
}

static void rcSelectChecksumCalculator(uint32_t protocol, uint32_t reserved) {
    ChecksumCalculatorThreadInfo::setVersion(protocol);
}

static uint64_t rcCreateSyncKHR(EGLenum type, EGLint* attribs, uint32_t num_attribs) {
    RenderThreadInfo *tInfo = RenderThreadInfo::get();
    RcSyncInfo* sync_info = RcSyncInfo::get();

    DPRINT("%s: type=0x%x num_attribs=%d\n",
            __FUNCTION__, type, num_attribs);

    GLsync sync;
    if (tInfo->currContext->isGL2()) {
        sync = s_gles2.glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
#if DEBUG
        int err = s_gles2.glGetError();
        if (err != GL_NO_ERROR || !sync) {
            fprintf(stderr, "%s: Error: glFenceSync returned sync=%p glerror=0x%x\n",
                    __FUNCTION__, (void*)sync, err);
        }
        DPRINT("%s: glerror=0x%x cxt@%p glsync@%p handle=0x%lx\n",
               __FUNCTION__,
               err,
               tInfo->currContext.get(),
               rcsync->sync,
               rcsync->handle);
#endif
        // This MUST be present, or we get a deadlock effect.
        // Still unsure whether this provides any guarantee
        // of ordering of fence commands.
        s_gles2.glFlush();
        RcSync* rcsync = new RcSync(sync, (void*)tInfo->currContext.get());
        sync_info->addSync(rcsync);
        return rcsync->handle;
    } else {
        DPRINT("%s: warning: CreateSync in gles1 context\n", __FUNCTION__);
        sync_info->addSync(NULL);
        return 0;
    }
}

static EGLint rcClientWaitSyncKHR(uint64_t handle, EGLint flags, uint64_t timeout) {
    RenderThreadInfo *tInfo = RenderThreadInfo::get();
    RenderContext* cxt;

    RcSyncInfo* sync_info = RcSyncInfo::get();
    DPRINT("%s: handle=0x%lx flags=0x%x timeout=%" PRIu64 "\n",
            __FUNCTION__, handle, flags, timeout);

    // If the fence was already signaled,
    // skip all this and go through the map instead.
    if (sync_info->isSignaled(handle)) {
        DPRINT("%s: fence with handle=0x%lx already signaled\n", __FUNCTION__, handle);
        return EGL_CONDITION_SATISFIED_KHR;
    }

    RcSync* rcsync = sync_info->lookupSync(handle);

    if (!rcsync) { DPRINT("%s: ERROR: rcsync is null!\n", __FUNCTION__); return EGL_CONDITION_SATISFIED_KHR; }

    GLsync glsync = rcsync->sync;
    RenderContext* orig_cxt = (RenderContext*)rcsync->gl_cxt;
    GLenum gl_wait_res;

    // Possibly big TODO: Need to have two threads for every context, one to render,
    // the other to wait on fences. This is for when the guest has not really
    // set up the second context itself.
    if (!tInfo->currContext) {
        DPRINT("%s: no context yet, creating\n", __FUNCTION__);
        FrameBuffer *fb = FrameBuffer::getFB();
        const FbConfig* cfg = fb->getConfigs()->get(0);
        cxt = RenderContext::create(fb->getDisplay(), cfg->getEglConfig(), orig_cxt->getEGLContext(), 1);
        DPRINT("%s: context created, cxt@%p err=0x%x\n", __FUNCTION__, cxt, s_egl.eglGetError());
        DPRINT("%s: cxt->eglcxt=%p origcxt=%p\n", __FUNCTION__, cxt->getEGLContext(), orig_cxt->getEGLContext());
        static const EGLint pbuf_attribs[] = {
            EGL_WIDTH, 1,
            EGL_HEIGHT, 1,
            EGL_NONE
        };
        EGLSurface surf = s_egl.eglCreatePbufferSurface(fb->getDisplay(), cfg->getEglConfig(), pbuf_attribs);
        s_egl.eglMakeCurrent(fb->getDisplay(), surf, surf, cxt->getEGLContext());
        DPRINT("%s: context current, cxt@%p err=0x%x\n", __FUNCTION__, cxt, s_egl.eglGetError());
    } else {
        cxt = tInfo->currContext.get();
    }

    if (cxt->isGL2()) {
        DPRINT("%s: calling with glsync@%p\n", __FUNCTION__, (void*)glsync);
        gl_wait_res = s_gles2.glClientWaitSync(glsync, GL_SYNC_FLUSH_COMMANDS_BIT, timeout);
    } else {
        DPRINT("%s: Warning: ClientWaitSync in gles1 context. Should be short circuited already!\n", __FUNCTION__);
        gl_wait_res = s_gles2.glClientWaitSync(glsync, GL_SYNC_FLUSH_COMMANDS_BIT, timeout);
    }

#if DEBUG
    int err = s_gles2.glGetError();
    if (err != GL_NO_ERROR) {
        fprintf(stderr, "%s: Error: glClientWaitSync returned 0x%x\n", __FUNCTION__, err);
    }
#endif

    EGLint egl_res;
    switch(gl_wait_res) {
    case GL_CONDITION_SATISFIED:
        DPRINT("%s: wait finished with GL_CONDITION_SATISFIED. res=0x%x glGetError=0x%x\n", __FUNCTION__, gl_wait_res, err);
        sync_info->setSignaled(handle);
        egl_res = EGL_CONDITION_SATISFIED_KHR;
        break;
    case GL_ALREADY_SIGNALED:
        DPRINT("%s: wait finished with GL_ALREADY_SIGNALED. res = 0x%x glGetError=0x%x\n", __FUNCTION__, gl_wait_res, err);
        sync_info->setSignaled(handle);
        egl_res = EGL_CONDITION_SATISFIED_KHR;
        break;
    case GL_TIMEOUT_EXPIRED:
        DPRINT("%s: wait finished with GL_TIMEOUT_EXPIRED. res = 0x%x glGetError=0x%x\n", __FUNCTION__, gl_wait_res, err);
        egl_res = EGL_TIMEOUT_EXPIRED_KHR;
        break;
    case GL_WAIT_FAILED:
        DPRINT("%s: wait finished with GL_WAIT_FAILED. res=0x%x glGetError=0x%x\n", __FUNCTION__, gl_wait_res, err);
        egl_res = EGL_CONDITION_SATISFIED_KHR;
        break;
    default:
        egl_res = EGL_CONDITION_SATISFIED_KHR;
    }
    return egl_res;
}

static EGLint rcWaitSyncKHR(uint64_t handle, EGLint flags) {
    RenderThreadInfo *tInfo = RenderThreadInfo::get();
    RenderContext* cxt;

    RcSyncInfo* sync_info = RcSyncInfo::get();
    DPRINT("%s: handle=0x%lx flags=0x%x timeout=%" PRIu64 "\n",
            __FUNCTION__, handle, flags, timeout);

    // If the fence was already signaled,
    // skip all this and go through the map instead.
    if (sync_info->isSignaled(handle)) {
        DPRINT("%s: fence with handle=0x%lx already signaled\n", __FUNCTION__, handle);
        return EGL_CONDITION_SATISFIED_KHR;
    }

    RcSync* rcsync = sync_info->lookupSync(handle);

    if (!rcsync) { DPRINT("%s: ERROR: rcsync is null!\n", __FUNCTION__); return EGL_CONDITION_SATISFIED_KHR; }

    GLsync glsync = rcsync->sync;

    cxt = tInfo->currContext.get();

    if (cxt->isGL2()) {
        DPRINT("%s: calling with glsync@%p\n", __FUNCTION__, (void*)glsync);
        s_gles2.glWaitSync(glsync, 0, GL_TIMEOUT_IGNORED);
    } else {
        DPRINT("%s: Warning: WaitSync in gles1 context. Should be short circuited already!\n", __FUNCTION__);
        s_gles2.glWaitSync(glsync, 0, GL_TIMEOUT_IGNORED);
    }

    int err = s_gles2.glGetError();
    if (err != GL_NO_ERROR) {
        fprintf(stderr, "%s: Error: glWaitSync returned 0x%x\n", __FUNCTION__, err);
    }
    return EGL_TRUE;
}

void initRenderControlContext(renderControl_decoder_context_t *dec)
{
    dec->rcGetRendererVersion = rcGetRendererVersion;
    dec->rcGetEGLVersion = rcGetEGLVersion;
    dec->rcQueryEGLString = rcQueryEGLString;
    dec->rcGetGLString = rcGetGLString;
    dec->rcGetNumConfigs = rcGetNumConfigs;
    dec->rcGetConfigs = rcGetConfigs;
    dec->rcChooseConfig = rcChooseConfig;
    dec->rcGetFBParam = rcGetFBParam;
    dec->rcCreateContext = rcCreateContext;
    dec->rcDestroyContext = rcDestroyContext;
    dec->rcCreateWindowSurface = rcCreateWindowSurface;
    dec->rcDestroyWindowSurface = rcDestroyWindowSurface;
    dec->rcCreateColorBuffer = rcCreateColorBuffer;
    dec->rcOpenColorBuffer = rcOpenColorBuffer;
    dec->rcCloseColorBuffer = rcCloseColorBuffer;
    dec->rcSetWindowColorBuffer = rcSetWindowColorBuffer;
    dec->rcFlushWindowColorBuffer = rcFlushWindowColorBuffer;
    dec->rcMakeCurrent = rcMakeCurrent;
    dec->rcFBPost = rcFBPost;
    dec->rcFBSetSwapInterval = rcFBSetSwapInterval;
    dec->rcBindTexture = rcBindTexture;
    dec->rcBindRenderbuffer = rcBindRenderbuffer;
    dec->rcColorBufferCacheFlush = rcColorBufferCacheFlush;
    dec->rcReadColorBuffer = rcReadColorBuffer;
    dec->rcUpdateColorBuffer = rcUpdateColorBuffer;
    dec->rcOpenColorBuffer2 = rcOpenColorBuffer2;
    dec->rcCreateClientImage = rcCreateClientImage;
    dec->rcDestroyClientImage = rcDestroyClientImage;
    dec->rcSelectChecksumCalculator = rcSelectChecksumCalculator;
    dec->rcCreateSyncKHR = rcCreateSyncKHR;
    dec->rcClientWaitSyncKHR = rcClientWaitSyncKHR;
    dec->rcWaitSyncKHR = rcWaitSyncKHR;
}
