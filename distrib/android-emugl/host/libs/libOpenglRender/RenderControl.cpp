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
#include "SyncThread.h"
#include "ChecksumCalculatorThreadInfo.h"
#include "OpenGLESDispatch/EGLDispatch.h"

#include "android/utils/debug.h"
#include "android/base/StringView.h"
#include "emugl/common/feature_control.h"
#include "emugl/common/lazy_instance.h"
#include "emugl/common/sync_device.h"
#include "emugl/common/thread.h"

#include <atomic>
#include <inttypes.h>
#include <string.h>

using android::base::AutoLock;
using android::base::Lock;

#define DEBUG_GRALLOC_SYNC 0

#define DEBUG_SYNC_THREADS 0

#if DEBUG_GRALLOC_SYNC
#define GRSYNC_DPRINT(...) do { \
    if (!VERBOSE_CHECK(gles)) { VERBOSE_ENABLE(gles); } \
    VERBOSE_TID_FUNCTION_DPRINT(gles, __VA_ARGS__); \
} while(0)
#else
#define GRSYNC_DPRINT(...)
#endif

#if DEBUG_SYNC_THREADS
#define SYNC_DPRINT(...) do { \
    if (!VERBOSE_CHECK(gles)) { VERBOSE_ENABLE(gles); } \
    VERBOSE_TID_FUNCTION_DPRINT(gles, __VA_ARGS__); \
} while(0)
#else
#define SYNC_DPRINT(...)
#endif

// GrallocSync is a class that helps to reflect the behavior of
// grallock_lock/gralloc_unlock on the guest.
// If we don't use this, apps that use gralloc buffers (such as webcam)
// will have out of order frames,
// as GL calls from different threads in the guest
// are allowed to arrive at the host in any ordering.
class GrallocSync {
public:
    GrallocSync() {
        // Having in-order webcam frames is nice, but not at the cost
        // of potential deadlocks;
        // we need to be careful of what situations in which
        // we actually lock/unlock the gralloc color buffer.
        //
        // To avoid deadlock:
        // we require rcColorBufferCacheFlush to be called
        // whenever gralloc_lock is called on the guest,
        // and we require rcUpdateWindowColorBuffer to be called
        // whenever gralloc_unlock is called on the guest.
        //
        // Some versions of the system image optimize out
        // the call to rcUpdateWindowColorBuffer in the case of zero
        // width/height, but since we're using that as synchronization,
        // that lack of calling can lead to a deadlock on the host
        // in many situations
        // (switching camera sides, exiting benchmark apps, etc)
        // So, we put GrallocSync under the feature control.
        mEnabled = emugl_feature_is_enabled(android::featurecontrol::GrallocSync);

        // There are two potential tricky situations to handle:
        // a. Multiple users of gralloc buffers that all want to
        // call gralloc_lock. This is obeserved to happen on older APIs
        // (<= 19).
        // b. The pipe doesn't have to preserve ordering of the
        // gralloc_lock and gralloc_unlock commands themselves.
        //
        // To handle a), notice the situation is one of one type of uses
        // needing multiple locks that needs to exclude concurrent use
        // by another type of user. This maps well to a read/write lock,
        // where gralloc_lock and gralloc_unlock users are readers
        // and rcFlushWindowColorBuffer is the writer.
        // From the perspective of the host preparing and posting
        // buffers, these are indeed read/write operations.
        //
        // To handle b), we give up on locking when the state is observed
        // to be bad. lockState tracks how many color buffer locks there are.
        // If lockState < 0, it means we definitely have an unlock before lock
        // sort of situation, and should give up.
        lockState = 0;
    }

    // lockColorBufferPrepare is designed to handle
    // gralloc_lock/unlock requests, and uses the read lock.
    // When rcFlushWindowColorBuffer is called (when frames are posted),
    // we use the write lock (see GrallocSyncPostLock).
    void lockColorBufferPrepare() {
        int newLockState = ++lockState;
        if (mEnabled && newLockState == 1) {
            mGrallocColorBufferLock.lockRead();
        } else if (mEnabled) {
            GRSYNC_DPRINT("warning: recursive/multiple locks from guest!");
        }
    }
    void unlockColorBufferPrepare() {
        int newLockState = --lockState;
        if (mEnabled && newLockState == 0) mGrallocColorBufferLock.unlockRead();
    }
    android::base::ReadWriteLock mGrallocColorBufferLock;
private:
    bool mEnabled;
    std::atomic<int> lockState;
    DISALLOW_COPY_ASSIGN_AND_MOVE(GrallocSync);
};

class GrallocSyncPostLock : public android::base::AutoWriteLock {
public:
    GrallocSyncPostLock(GrallocSync& grallocsync) :
        android::base::AutoWriteLock(grallocsync.mGrallocColorBufferLock) { }
};

static ::emugl::LazyInstance<GrallocSync> sGrallocSync = LAZY_INSTANCE_INIT;

static const GLint rendererVersion = 1;
static android::base::StringView kAsyncSwapStr = "ANDROID_EMU_ASYNC_SWAP";

static void rcTriggerWait(uint64_t glsync_ptr,
                          uint64_t thread_ptr,
                          uint64_t timeline);

static GLint rcGetRendererVersion()
{
    emugl_sync_register_trigger_wait(rcTriggerWait);

    sGrallocSync.ptr();
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

static EGLint rcGetGLString(EGLenum name, void* buffer, EGLint bufferSize)
{
    RenderThreadInfo *tInfo = RenderThreadInfo::get();

    // whatever we end up returning,
    // it will have a terminating \0,
    // so account for it here.
    std::string glStr;

    if (tInfo && tInfo->currContext.get()) {
        const char *str = nullptr;
        if (tInfo->currContext->isGL2()) {
            str = (const char *)s_gles2.glGetString(name);
        }
        else {
            str = (const char *)s_gles1.glGetString(name);
        }
        if (str) {
            glStr += str;
        }
    }

    // We add the maximum supported GL protocol number into GL_EXTENSIONS
    bool isChecksumEnabled =
        emugl_feature_is_enabled(android::featurecontrol::GLPipeChecksum);
    bool asyncSwapEnabled =
        emugl_feature_is_enabled(android::featurecontrol::GLAsyncSwap);

    if (isChecksumEnabled && name == GL_EXTENSIONS) {
        glStr += ChecksumCalculatorThreadInfo::getMaxVersionString();
        glStr += " ";
    }

    if (asyncSwapEnabled && name == GL_EXTENSIONS) {
        glStr += kAsyncSwapStr;
        glStr += " ";
    }

    int nextBufferSize = glStr.size() + 1;

    if (!buffer || nextBufferSize > bufferSize) {
        return -nextBufferSize;
    }

    snprintf((char *)buffer, nextBufferSize, "%s", glStr.c_str());
    return nextBufferSize;
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
    // destroy_sync_thread();

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

// static int triggerSyncThreadAndCreateFenceFd() {
//     RenderThreadInfo *tInfo = RenderThreadInfo::get();
// 
//     if (tInfo->syncThread
// 
// }

static int rcFlushWindowColorBuffer(uint32_t windowSurface)
{
    GRSYNC_DPRINT("waiting for gralloc cb lock");
    GrallocSyncPostLock lock(sGrallocSync.get());
    GRSYNC_DPRINT("lock gralloc cb lock {");

    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        GRSYNC_DPRINT("unlock gralloc cb lock");
        return -1;
    }
    if (!fb->flushWindowSurfaceColorBuffer(windowSurface)) {
        GRSYNC_DPRINT("unlock gralloc cb lock }");
        return -1;
    }

    GRSYNC_DPRINT("unlock gralloc cb lock }");

    // int fenceFd = triggerSyncThreadAndCreateFenceFd();

    return 0;
}

// Note that even though this calls rcFlushWindowColorBuffer,
// the "Async" part is in the return type, which is void
// versus return type int for rcFlushWindowColorBuffer.
//
// In fact, if the return type is int, we will wait on the guest
// until the function returns, and then get traffic
// (consisting of return value) back to the guest,
// and only then will guest proceed.
//
// rcFlushWindowColorBufferAsync is designed to not
// incur this traffic cost: with return type void,
// the guest will not wait until this function returns,
// nor will it immediately send the command,
// resultign in more asynchronous behavior.
static void rcFlushWindowColorBufferAsync(uint32_t windowSurface)
{
    rcFlushWindowColorBuffer(windowSurface);
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
   // gralloc_lock() on the guest calls rcColorBufferCacheFlush
   GRSYNC_DPRINT("waiting for gralloc cb lock");
   sGrallocSync->lockColorBufferPrepare();
   GRSYNC_DPRINT("lock gralloc cb lock {");
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
        GRSYNC_DPRINT("unlock gralloc cb lock");
        sGrallocSync->unlockColorBufferPrepare();
        return -1;
    }

    fb->updateColorBuffer(colorBuffer, x, y, width, height, format, type, pixels);

    GRSYNC_DPRINT("unlock gralloc cb lock");
    sGrallocSync->unlockColorBufferPrepare();
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

static void rcSelectChecksumHelper(uint32_t protocol, uint32_t reserved) {
    ChecksumCalculatorThreadInfo::setVersion(protocol);
}

static void get_sync_error(GLsync sync,
                           RenderContext* cxt,
                           RcSync* rcsync,
                           GLint* err) {
#if DEBUG
    *err = s_gles2.glGetError();
    if (err != GL_NO_ERROR || !sync) {
        fprintf(stderr, "%s: Error: glFenceSync returned sync=%p glerror=0x%x\n",
                func_name, (void*)sync, err);
    }
    SYNC_DPRINT("glerror=0x%x cxt@%p glsync@%p handle=0x%lx\n",
                err,
                cxt,
                rcsync->sync,
                rcsync->getHandle());
#endif
}

static void rcTriggerWait(uint64_t glsync_ptr,
                          uint64_t thread_ptr,
                          uint64_t timeline) {
    RcSyncInfo* sync_info = RcSyncInfo::get();
    RcSync* rcsync = sync_info->findSync(glsync_ptr);
    SYNC_DPRINT("glsync=0x%llx nextglsync=0x%llx thread_ptr=0x%llx timeline=0x%llx",
                glsync_ptr, rcsync, thread_ptr, timeline);
    SyncThread* syncThread = (SyncThread*)(uintptr_t)thread_ptr;
    syncThread->triggerWait(rcsync, timeline);
}

static uint64_t rcCreateSyncKHR(EGLenum type, EGLint* attribs, uint32_t num_attribs, uint64_t* glsync_out, uint64_t* syncthread_out) {

    RcSyncInfo* sync_info = RcSyncInfo::get();

    if (glsync_out) *glsync_out = 0;
    if (syncthread_out) *syncthread_out = 0;

    SyncThread* syncthread = get_sync_thread();

    if (syncthread_out) *syncthread_out = (uint64_t)(uintptr_t)(syncthread);

    SYNC_DPRINT("type=0x%x num_attribs=%d",
                type, num_attribs);

    if (RenderThreadInfo::get()->currContext->isGL2()) {
        GLsync sync = s_gles2.glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        SYNC_DPRINT("got gl sync: 0x%llx", sync);
        RcSync* rcsync = new RcSync(sync, (void*)RenderThreadInfo::get()->currContext.get());
        sync_info->addSync(rcsync);

        GLint err;
        get_sync_error(sync, RenderThreadInfo::get()->currContext.get(), rcsync, &err);
        // This MUST be present, or we get a deadlock effect.
        // Still unsure whether this provides any guarantee
        // of ordering of fence commands.
        s_gles2.glFlush();
        uint64_t res = rcsync->getHandle();
        SYNC_DPRINT("send out glsync 0x%llx", res);
        if (glsync_out) *glsync_out = res;
        return res;
    } else {
        SYNC_DPRINT("warning: CreateSync in gles1 context");
        sync_info->addSync(nullptr);
        return 0;
    }
}

static EGLint rcClientWaitSyncKHR(uint64_t handle, EGLint flags, uint64_t timeout) {
    RenderThreadInfo *tInfo = RenderThreadInfo::get();
    RenderContext* cxt;

    RcSyncInfo* sync_info = RcSyncInfo::get();
    SYNC_DPRINT("handle=0x%lx flags=0x%x timeout=%" PRIu64,
                handle, flags, timeout);

    RcSync* rcsync = sync_info->findNonSignaledSync(handle);

    if (!rcsync) {
        SYNC_DPRINT("rcsync null, return condition satisfied");
        return EGL_CONDITION_SATISFIED_KHR;
    }

    GLsync glsync = rcsync->sync;
    RenderContext* orig_cxt = (RenderContext*)rcsync->gl_cxt;
    GLenum gl_wait_res;


    // Possibly big TODO: Need to have two threads for every context, one to render,
    // the other to wait on fences. This is for when the guest has not really
    // set up the second context itself.
    if (!tInfo->currContext) {
        SYNC_DPRINT("no context yet, creating");
        FrameBuffer *fb = FrameBuffer::getFB();
        const FbConfig* cfg = fb->getConfigs()->get(0);
        cxt = RenderContext::create(fb->getDisplay(), cfg->getEglConfig(), orig_cxt->getEGLContext(), 1);
        SYNC_DPRINT("context created, cxt@%p err=0x%x", cxt, s_egl.eglGetError());
        SYNC_DPRINT("cxt->eglcxt=%p origcxt=%p", cxt->getEGLContext(), orig_cxt->getEGLContext());
        static const EGLint pbuf_attribs[] = {
            EGL_WIDTH, 1,
            EGL_HEIGHT, 1,
            EGL_NONE
        };
        EGLSurface surf = s_egl.eglCreatePbufferSurface(fb->getDisplay(), cfg->getEglConfig(), pbuf_attribs);
        s_egl.eglMakeCurrent(fb->getDisplay(), surf, surf, cxt->getEGLContext());
        SYNC_DPRINT("context current, cxt@%p err=0x%x", cxt, s_egl.eglGetError());
    } else {
        cxt = tInfo->currContext.get();
    }

    if (cxt->isGL2()) {
        SYNC_DPRINT("calling with glsync@%p", (void*)glsync);
        gl_wait_res = s_gles2.glClientWaitSync(glsync, GL_SYNC_FLUSH_COMMANDS_BIT, timeout);
    } else {
        SYNC_DPRINT("Warning: ClientWaitSync in gles1 context. Should be short circuited already!");
        gl_wait_res = s_gles2.glClientWaitSync(glsync, GL_SYNC_FLUSH_COMMANDS_BIT, timeout);
    }

    GLint err = GL_NO_ERROR;

    get_sync_error(glsync, cxt, rcsync, &err);

    EGLint egl_res;
    switch(gl_wait_res) {
    case GL_CONDITION_SATISFIED:
        SYNC_DPRINT("wait finished with GL_CONDITION_SATISFIED. res=0x%x glGetError=0x%x", gl_wait_res, err);
        sync_info->setSignaled(handle);
        egl_res = EGL_CONDITION_SATISFIED_KHR;
        break;
    case GL_ALREADY_SIGNALED:
        SYNC_DPRINT("wait finished with GL_ALREADY_SIGNALED. res = 0x%x glGetError=0x%x", gl_wait_res, err);
        sync_info->setSignaled(handle);
        egl_res = EGL_CONDITION_SATISFIED_KHR;
        break;
    case GL_TIMEOUT_EXPIRED:
        SYNC_DPRINT("wait finished with GL_TIMEOUT_EXPIRED. res = 0x%x glGetError=0x%x", gl_wait_res, err);
        egl_res = EGL_TIMEOUT_EXPIRED_KHR;
        break;
    case GL_WAIT_FAILED:
        SYNC_DPRINT("wait finished with GL_WAIT_FAILED. res=0x%x glGetError=0x%x", gl_wait_res, err);
        egl_res = EGL_CONDITION_SATISFIED_KHR;
        break;
    default:
        egl_res = EGL_CONDITION_SATISFIED_KHR;
    }
    return egl_res;
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
    dec->rcSelectChecksumHelper = rcSelectChecksumHelper;
    dec->rcCreateSyncKHR = rcCreateSyncKHR;
    dec->rcClientWaitSyncKHR = rcClientWaitSyncKHR;
    dec->rcFlushWindowColorBufferAsync = rcFlushWindowColorBufferAsync;
}


