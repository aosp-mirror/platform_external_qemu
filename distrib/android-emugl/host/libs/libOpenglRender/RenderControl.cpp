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
#include "FenceSync.h"
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
#include "emugl/common/dma_device.h"
#include "emugl/common/thread.h"

#include <atomic>
#include <inttypes.h>
#include <string.h>

using android::base::AutoLock;
using android::base::Lock;

#define DEBUG_GRALLOC_SYNC 0
#define DEBUG_EGL_SYNC 0

#define RENDERCONTROL_DPRINT(...) do { \
    if (!VERBOSE_CHECK(gles)) { VERBOSE_ENABLE(gles); } \
    VERBOSE_TID_FUNCTION_DPRINT(gles, __VA_ARGS__); \
} while(0)

#if DEBUG_GRALLOC_SYNC
#define GRSYNC_DPRINT RENDERCONTROL_DPRINT
#else
#define GRSYNC_DPRINT(...)
#endif

#if DEBUG_EGL_SYNC
#define EGLSYNC_DPRINT RENDERCONTROL_DPRINT
#else
#define EGLSYNC_DPRINT(...)
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

    void setSync() {
        mSync = s_gles2.glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    }

    void waitSync() {
        if (mSync) {
            s_gles2.glClientWaitSync(mSync, GL_SYNC_FLUSH_COMMANDS_BIT, 3000000000);
            s_gles2.glDeleteSync(mSync);
            mSync = NULL;
        }
    }

    GLsync mSync = NULL;
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

// GLAsyncSwap version history:
// "ANDROID_EMU_NATIVE_SYNC": original version
// "ANDROIDEMU_native_sync_v2": +cleanup of sync objects
// (We need all the different strings to not be prefixes of any other
// due to how they are checked for in the GL extensions on the guest)
static android::base::StringView kAsyncSwapStr = "ANDROID_EMU_native_sync_v2";

static void rcTriggerWait(uint64_t glsync_ptr,
                          uint64_t thread_ptr,
                          uint64_t timeline);

static void* emuglDMA = NULL;
static GLint rcGetRendererVersion()
{
    emugl_sync_register_trigger_wait(rcTriggerWait);
    emuglDMA = dma_ptr_getter();

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

    std::string eglStr(str);

    int len = eglStr.size() + 1;
    if (!buffer || len > bufferSize) {
        return -len;
    }

    strcpy((char *)buffer, eglStr.c_str());
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
        emugl_feature_is_enabled(android::featurecontrol::GLAsyncSwap) &&
        emugl_sync_device_exists();

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
    SyncThread::destroySyncThread();

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

    return 0;
}

// Note that even though this calls rcFlushWindowColorBuffer,
// the "Async" part is in the return type, which is void
// versus return type int for rcFlushWindowColorBuffer.
//
// The different return type, even while calling the same
// functions internally, will end up making the encoder
// and decoder use a different protocol. This is because
// the encoder generally obeys the following conventions:
//
// - The encoder will immediately send and wait for a command
//   result if the return type is not void.
// - The encoder will cache the command in a buffer and send
//   at a convenient time if the return type is void.
//
// It can also be expensive performance-wise to trigger
// sending traffic back to the guest. Generally, the more we avoid
// encoding commands that perform two-way traffic, the better.
//
// Hence, |rcFlushWindowColorBufferAsync| will avoid extra traffic;
// with return type void,
// the guest will not wait until this function returns,
// nor will it immediately send the command,
// resulting in more asynchronous behavior.
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

static signed clamp_rgb(signed value) {
    if (value > 255) {
        value = 255;
    } else if (value < 0) {
        value = 0;
    }
    return value;
}

// YV12 is aka YUV420Planar, or YUV420p; the only difference is that YV12 has
// certain stride requirements for Y and UV respectively.
// static void yv12_to_rgb888(char* dest, char* src, int width, int height,
//         int left, int top, int right, int bottom) {
//     int align = 16;
//     int yStride = (width + (align -1)) & ~(align-1);
//     int cStride = (yStride / 2 + (align - 1)) & ~(align-1);
//     int cSize = cStride * height/2;
//     int rgb_stride = 3;
// 
//     uint8_t *rgb_ptr0 = (uint8_t *)dest;
//     uint8_t *yv12_y0 = (uint8_t *)src;
//     uint8_t *yv12_v0 = yv12_y0 + yStride * height;
// 
//     for (int j = top; j <= bottom; ++j) {
//         uint8_t *yv12_y = yv12_y0 + j * yStride;
//         uint8_t *yv12_v = yv12_v0 + (j/2) * cStride;
//         uint8_t *yv12_u = yv12_v + cSize;
//         uint8_t *rgb_ptr = rgb_ptr0 + (j-top) * (right-left+1) * rgb_stride;
//         for (int i = left; i <= right; ++i) {
//             // convert to rgb
//             // frameworks/av/media/libstagefright/colorconversion/ColorConverter.cpp
//             signed y1 = (signed)yv12_y[i] - 16;
//             signed u = (signed)yv12_u[i / 2] - 128;
//             signed v = (signed)yv12_v[i / 2] - 128;
// 
//             signed u_b = u * 517;
//             signed u_g = -u * 100;
//             signed v_g = -v * 208;
//             signed v_r = v * 409;
// 
//             signed tmp1 = y1 * 298;
//             signed b1 = clamp_rgb((tmp1 + u_b) / 256);
//             signed g1 = clamp_rgb((tmp1 + v_g + u_g) / 256);
//             signed r1 = clamp_rgb((tmp1 + v_r) / 256);
// 
//             rgb_ptr[(i-left)*rgb_stride] = r1;
//             rgb_ptr[(i-left)*rgb_stride+1] = g1;
//             rgb_ptr[(i-left)*rgb_stride+2] = b1;
//         }
//     }
// }

#define ANDROID_EMU_YV12 0x666

static size_t updateWindowSize = 60;
static std::vector<int> YVConvertTimes;
static std::vector<int> TexSubImageTimes;
static int maxYVConvertTime;
static int maxTexSubImageTime;
static float avgYVConvertTime;
static float avgTexSubImageTime;
static size_t updateWindowIndex = 0;

#include <sys/time.h>
static int currTimeMs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int)(tv.tv_usec / 1000 + tv.tv_sec * 1000);
}

static void rcUpdateColorBuffer(uint32_t colorBuffer,
                                GLint x, GLint y,
                                GLint width, GLint height,
                                GLenum format, GLenum type,
                                void* pixels)
{
    GrallocSyncPostLock lock(sGrallocSync.get());

    if (YVConvertTimes.size() < updateWindowSize) {
        YVConvertTimes.resize(updateWindowSize);
    }

    if (TexSubImageTimes.size() < updateWindowSize) {
        TexSubImageTimes.resize(updateWindowSize);
    }

    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        GRSYNC_DPRINT("unlock gralloc cb lock");
        sGrallocSync->unlockColorBufferPrepare();
        return;
    }

    int convStart = currTimeMs();
    GLenum usedFormat = format;
    void* toSend = emuglDMA;
    bool doConvert = false;
    if (format == ANDROID_EMU_YV12) {
        usedFormat = GL_RGB;
        doConvert = true;

        uint32_t align = 16;

        uint32_t yStride = (width + (align - 1)) & ~(align - 1);
        uint32_t uvStride = (yStride / 2 + (align - 1)) & ~(align - 1);
        uint32_t uvHeight = height / 2;
        uint32_t total_size = 
            yStride * height + 2 * (uvHeight * uvStride);
        memcpy(fb->conversionBuffer,
               emuglDMA,
               total_size);

        // yv12_to_rgb888((char*)(fb->conversionBuffer) + total_size,
        //                fb->conversionBuffer,
        //                width, height, 
        //                x, y,
        //                x + width - 1,
        //                y + height - 1);
        toSend = (char*)(fb->conversionBuffer) + total_size;
    }

    int convEnd = currTimeMs();

    int texsubStart = currTimeMs();
    fb->updateColorBuffer(colorBuffer, x, y, width, height, usedFormat, type, doConvert, toSend);
    int texsubEnd = currTimeMs();

    YVConvertTimes[updateWindowIndex] = convEnd - convStart;
    TexSubImageTimes[updateWindowIndex] = texsubEnd - texsubStart;

    GRSYNC_DPRINT("unlock gralloc cb lock");
    sGrallocSync->unlockColorBufferPrepare();

    updateWindowIndex++;
    if (updateWindowIndex >= updateWindowSize) {
        updateWindowIndex = 0;
        maxYVConvertTime = 0;
        maxTexSubImageTime = 0;
        avgYVConvertTime = 0.0f;
        avgTexSubImageTime = 0.0f;
        for (size_t i = 0; i < updateWindowSize; i++) {
            if (YVConvertTimes[i] > maxYVConvertTime) {
                maxYVConvertTime = YVConvertTimes[i];
            }
            avgYVConvertTime += (float)YVConvertTimes[i];
            if (TexSubImageTimes[i] > maxTexSubImageTime) {
                maxTexSubImageTime = TexSubImageTimes[i];
            }
            avgTexSubImageTime += (float)TexSubImageTimes[i];
        }

        avgYVConvertTime /= (float)updateWindowSize;
        avgTexSubImageTime /= (float)updateWindowSize;

        fprintf(stderr, "%s: maxyvconvert %d maxtexsub %d avgyvconvert %f avgtexsub %f\n",
                __FUNCTION__, maxYVConvertTime, maxTexSubImageTime,
                avgYVConvertTime, avgTexSubImageTime);
    }

    return;
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

// |rcTriggerWait| is called from the goldfish sync
// kernel driver whenever a native fence fd is created.
// We will then need to use the host to find out
// when to signal that native fence fd. We use
// SyncThread for that.
//
// The purpose of |rcTriggerWait| is to tell which
// SyncThread which sync object / timeline to signal.
static void rcTriggerWait(uint64_t eglsync_ptr,
                          uint64_t thread_ptr,
                          uint64_t timeline) {
    FenceSync* fenceSync = (FenceSync*)(uintptr_t)eglsync_ptr;
    EGLSYNC_DPRINT("eglsync=0x%llx "
                   "fenceSync=%p "
                   "thread_ptr=0x%llx "
                   "timeline=0x%llx",
                   eglsync_ptr, fenceSync, thread_ptr, timeline);
    SyncThread* syncThread =
        reinterpret_cast<SyncThread*>(thread_ptr);
    syncThread->triggerWait(fenceSync, timeline);
}

// |rcCreateSyncKHR| implements the guest's
// |eglCreateSyncKHR| by calling the host's implementation
// of |eglCreateSyncKHR|. A SyncThread is also started
// that corresponds to the current rendering thread, for
// purposes of signaling any native fence fd's that
// get created in the guest off the sync object
// created here.
static void rcCreateSyncKHR(EGLenum type,
                            EGLint* attribs,
                            uint32_t num_attribs,
                            int destroy_when_signaled,
                            uint64_t* eglsync_out,
                            uint64_t* syncthread_out) {
    EGLSYNC_DPRINT("type=0x%x num_attribs=%d",
            type, num_attribs);

    bool hasNativeFence =
        type == EGL_SYNC_NATIVE_FENCE_ANDROID;

    FenceSync* fenceSync = new FenceSync(hasNativeFence,
                                         destroy_when_signaled);

    // This MUST be present, or we get a deadlock effect.
    s_gles2.glFlush();

    if (syncthread_out) *syncthread_out =
        reinterpret_cast<uint64_t>(SyncThread::getSyncThread());

    if (eglsync_out) {
        uint64_t res = (uint64_t)(uintptr_t)fenceSync;
        *eglsync_out = res;
        EGLSYNC_DPRINT("send out eglsync 0x%llx", res);
    }
}

// |rcClientWaitSyncKHR| implements |eglClientWaitSyncKHR|
// on the guest through using the host's existing
// |eglClientWaitSyncKHR| implementation, which is done
// through the FenceSync object.
static EGLint rcClientWaitSyncKHR(uint64_t handle,
                                  EGLint flags,
                                  uint64_t timeout) {
    RenderThreadInfo *tInfo = RenderThreadInfo::get();
    FrameBuffer *fb = FrameBuffer::getFB();

    EGLSYNC_DPRINT("handle=0x%lx flags=0x%x timeout=%" PRIu64,
                handle, flags, timeout);

    FenceSync* fenceSync = (FenceSync*)(uintptr_t)handle;

    if (!fenceSync) {
        EGLSYNC_DPRINT("fenceSync null, return condition satisfied");
        return EGL_CONDITION_SATISFIED_KHR;
    }

    // Sometimes a gralloc-buffer-only thread is doing stuff with sync.
    // This happens all the time with YouTube videos in the browser.
    // In this case, create a context on the host just for syncing.
    if (!tInfo->currContext) {
        uint32_t gralloc_sync_cxt, gralloc_sync_surf;
        fb->createTrivialContext(0, // There is no context to share.
                                 &gralloc_sync_cxt,
                                 &gralloc_sync_surf);
        fb->bindContext(gralloc_sync_cxt,
                        gralloc_sync_surf,
                        gralloc_sync_surf);
        // This context is then cleaned up when the render thread exits.
    }

    return fenceSync->wait(timeout);
}

static int rcDestroySyncKHR(uint64_t handle) {
    FenceSync* fenceSync = (FenceSync*)(uintptr_t)handle;
    assert(fenceSync);
    fenceSync->decRef();
    return 0;
}

static void rcSetPuid(uint64_t puid) {
    RenderThreadInfo *tInfo = RenderThreadInfo::get();
    tInfo->m_puid = puid;
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
    dec->rcDestroySyncKHR = rcDestroySyncKHR;
    dec->rcSetPuid = rcSetPuid;
}
