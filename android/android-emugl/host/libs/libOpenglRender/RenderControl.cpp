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
#include "GLESVersionDetector.h"
#include "RenderContext.h"
#include "RenderThreadInfo.h"
#include "SyncThread.h"
#include "ChecksumCalculatorThreadInfo.h"
#include "OpenGLESDispatch/EGLDispatch.h"
#include "vulkan/VkCommonOperations.h"
#include "vulkan/VkDecoderGlobalState.h"

#include "android/utils/debug.h"
#include "android/base/StringView.h"
#include "android/base/Tracing.h"
#include "emugl/common/feature_control.h"
#include "emugl/common/lazy_instance.h"
#include "emugl/common/sync_device.h"
#include "emugl/common/dma_device.h"
#include "emugl/common/misc.h"
#include "emugl/common/thread.h"
#include "math.h"

#include <atomic>
#include <inttypes.h>
#include <string.h>

using android::base::AutoLock;
using android::base::Lock;
using emugl::emugl_feature_is_enabled;
using emugl::emugl_sync_device_exists;
using emugl::emugl_sync_register_trigger_wait;

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
// "ANDROIDEMU_native_sync_v3": EGL_KHR_wait_sync
// (We need all the different strings to not be prefixes of any other
// due to how they are checked for in the GL extensions on the guest)
static constexpr android::base::StringView kAsyncSwapStrV2 = "ANDROID_EMU_native_sync_v2";
static constexpr android::base::StringView kAsyncSwapStrV3 = "ANDROID_EMU_native_sync_v3";

// DMA version history:
// "ANDROID_EMU_dma_v1": add dma device and rcUpdateColorBufferDMA and do
// yv12 conversion on the GPU
// "ANDROID_EMU_dma_v2": adds DMA support glMapBufferRange (and unmap)
static constexpr android::base::StringView kDma1Str = "ANDROID_EMU_dma_v1";
static constexpr android::base::StringView kDma2Str = "ANDROID_EMU_dma_v2";
static constexpr android::base::StringView kDirectMemStr = "ANDROID_EMU_direct_mem";

// GLESDynamicVersion: up to 3.1 so far
static constexpr android::base::StringView kGLESDynamicVersion_2 = "ANDROID_EMU_gles_max_version_2";
static constexpr android::base::StringView kGLESDynamicVersion_3_0 = "ANDROID_EMU_gles_max_version_3_0";
static constexpr android::base::StringView kGLESDynamicVersion_3_1 = "ANDROID_EMU_gles_max_version_3_1";

// HWComposer Host Composition
static constexpr android::base::StringView kHostCompositionV1 = "ANDROID_EMU_host_composition_v1";

static constexpr android::base::StringView kGLESNoHostError = "ANDROID_EMU_gles_no_host_error";

// Vulkan
static constexpr android::base::StringView kVulkanFeatureStr = "ANDROID_EMU_vulkan";
static constexpr android::base::StringView kDeferredVulkanCommands = "ANDROID_EMU_deferred_vulkan_commands";

static void rcTriggerWait(uint64_t glsync_ptr,
                          uint64_t thread_ptr,
                          uint64_t timeline);

void registerTriggerWait() {
    emugl_sync_register_trigger_wait(rcTriggerWait);
}

static GLint rcGetRendererVersion()
{
    registerTriggerWait();

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
    if ((FrameBuffer::getMaxGLESVersion() >= GLES_DISPATCH_MAX_VERSION_3_0) &&
        emugl_feature_is_enabled(android::featurecontrol::GLESDynamicVersion) &&
        eglStr.find("EGL_KHR_create_context") == std::string::npos) {
        eglStr += "EGL_KHR_create_context ";
    }

    int len = eglStr.size() + 1;
    if (!buffer || len > bufferSize) {
        return -len;
    }

    strcpy((char *)buffer, eglStr.c_str());
    return len;
}

static bool shouldEnableAsyncSwap() {
    bool isPhone;
    emugl::getAvdInfo(&isPhone, NULL);
    return emugl_feature_is_enabled(android::featurecontrol::GLAsyncSwap) &&
           emugl_sync_device_exists() &&
           isPhone &&
           sizeof(void*) == 8;
}

static bool shouldEnableHostComposition() {
    return emugl_feature_is_enabled(android::featurecontrol::HostComposition);
}

static bool shouldEnableVulkan() {
    auto supportInfo =
        goldfish_vk::VkDecoderGlobalState::get()->
            getHostFeatureSupport();
    bool flagEnabled =
        emugl_feature_is_enabled(android::featurecontrol::Vulkan);
    // TODO: Restrict further to devices supporting external memory.
    return supportInfo.supportsVulkan &&
           flagEnabled;
}

static bool shouldEnableDeferredVulkanCommands() {
    auto supportInfo =
        goldfish_vk::VkDecoderGlobalState::get()->
            getHostFeatureSupport();
    return supportInfo.supportsVulkan &&
           supportInfo.useDeferredCommands;
}

android::base::StringView maxVersionToFeatureString(GLESDispatchMaxVersion version) {
    switch (version) {
        case GLES_DISPATCH_MAX_VERSION_2:
            return kGLESDynamicVersion_2;
        case GLES_DISPATCH_MAX_VERSION_3_0:
            return kGLESDynamicVersion_3_0;
        case GLES_DISPATCH_MAX_VERSION_3_1:
            return kGLESDynamicVersion_3_1;
        default:
            return kGLESDynamicVersion_2;
    }
}

// OpenGL ES 3.x support involves changing the GL_VERSION string, which is
// assumed to be formatted in the following way:
// "OpenGL ES-CM 1.m <vendor-info>" or
// "OpenGL ES M.m <vendor-info>"
// where M is the major version number and m is minor version number.  If the
// GL_VERSION string doesn't reflect the maximum available version of OpenGL
// ES, many apps will not be able to detect support.  We need to mess with the
// version string in the first place since the underlying backend (whether it
// is Translator, SwiftShader, ANGLE, et al) may not advertise a GL_VERSION
// string reflecting their maximum capabilities.
std::string replaceESVersionString(const std::string& prev,
                                   android::base::StringView newver) {

    // There is no need to fiddle with the string
    // if we are in a ES 1.x context.
    // Such contexts are considered as a special case that must
    // be untouched.
    if (prev.find("ES-CM") != std::string::npos) {
        return prev;
    }

    size_t esStart = prev.find("ES ");
    size_t esEnd = prev.find(" ", esStart + 3);

    if (esStart == std::string::npos ||
        esEnd == std::string::npos) {
        // Account for out-of-spec version strings.
        fprintf(stderr, "%s: Error: invalid OpenGL ES version string %s\n",
                __func__, prev.c_str());
        return prev;
    }

    std::string res = prev.substr(0, esStart + 3);
    res += newver;
    res += prev.substr(esEnd);

    return res;
}

// If the GLES3 feature is disabled, we also want to splice out
// OpenGL extensions that should not appear in a GLES2 system.
void removeExtension(std::string& currExts, const std::string& toRemove) {
    size_t pos = currExts.find(toRemove);

    if (pos != std::string::npos)
        currExts.erase(pos, toRemove.length());
}

static EGLint rcGetGLString(EGLenum name, void* buffer, EGLint bufferSize) {
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    RenderThreadInfo *tInfo = RenderThreadInfo::get();

    // whatever we end up returning,
    // it will have a terminating \0,
    // so account for it here.
    std::string glStr;

    if (tInfo && tInfo->currContext.get()) {
        const char *str = nullptr;
        if (tInfo->currContext->clientVersion() > GLESApi_CM) {
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

    // filter extensions by name to match guest-side support
    GLESDispatchMaxVersion maxVersion = FrameBuffer::getMaxGLESVersion();
    if (name == GL_EXTENSIONS) {
        glStr = filterExtensionsBasedOnMaxVersion(maxVersion, glStr);
    }

    bool isChecksumEnabled =
        emugl_feature_is_enabled(android::featurecontrol::GLPipeChecksum);
    bool asyncSwapEnabled = shouldEnableAsyncSwap();
    bool dma1Enabled =
        emugl_feature_is_enabled(android::featurecontrol::GLDMA);
    bool dma2Enabled =
        emugl_feature_is_enabled(android::featurecontrol::GLDMA2);
    bool directMemEnabled =
        emugl_feature_is_enabled(android::featurecontrol::GLDirectMem);
    bool hostCompositionEnabled = shouldEnableHostComposition();
    bool vulkanEnabled = shouldEnableVulkan();
    bool deferredVulkanCommandsEnabled =
        shouldEnableVulkan() && shouldEnableDeferredVulkanCommands();

    if (isChecksumEnabled && name == GL_EXTENSIONS) {
        glStr += ChecksumCalculatorThreadInfo::getMaxVersionString();
        glStr += " ";
    }

    if (asyncSwapEnabled && name == GL_EXTENSIONS) {
        glStr += kAsyncSwapStrV2;
        glStr += " "; // for compatibility with older system images
        // Only enable EGL_KHR_wait_sync for host gpu.
        if (emugl::getRenderer() == SELECTED_RENDERER_HOST) {
            glStr += kAsyncSwapStrV3;
            glStr += " ";
        }
    }

    if (dma1Enabled && name == GL_EXTENSIONS) {
        glStr += kDma1Str;
        glStr += " ";
    }

    if (dma2Enabled && name == GL_EXTENSIONS) {
        glStr += kDma2Str;
        glStr += " ";
    }

    if (directMemEnabled && name == GL_EXTENSIONS) {
        glStr += kDirectMemStr;
        glStr += " ";
    }

    if (hostCompositionEnabled && name == GL_EXTENSIONS) {
        glStr += kHostCompositionV1;
        glStr += " ";
    }

    if (vulkanEnabled && name == GL_EXTENSIONS) {
        glStr += kVulkanFeatureStr;
        glStr += " ";
    }

    if (deferredVulkanCommandsEnabled && name == GL_EXTENSIONS) {
        glStr += kDeferredVulkanCommands;
        glStr += " ";
    }

    if (name == GL_EXTENSIONS) {

        GLESDispatchMaxVersion guestExtVer = GLES_DISPATCH_MAX_VERSION_2;
        if (emugl_feature_is_enabled(android::featurecontrol::GLESDynamicVersion)) {
            // If the image is in ES 3 mode, add GL_OES_EGL_image_external_essl3 for better Skia support.
            glStr += "GL_OES_EGL_image_external_essl3 ";
            guestExtVer = maxVersion;
        }

        // If we have a GLES3 implementation, add the corresponding
        // GLESv2 extensions as well.
        if (maxVersion > GLES_DISPATCH_MAX_VERSION_2) {
            glStr += "GL_OES_vertex_array_object ";
        }

        // ASTC LDR compressed texture support.
        glStr += "GL_KHR_texture_compression_astc_ldr ";

        if (emugl_feature_is_enabled(android::featurecontrol::IgnoreHostOpenGLErrors)) {
            glStr += kGLESNoHostError;
            glStr += " ";
        }

        glStr += maxVersionToFeatureString(guestExtVer);
        glStr += " ";
    }

    if (name == GL_VERSION) {
        if (emugl_feature_is_enabled(android::featurecontrol::GLESDynamicVersion)) {
            GLESDispatchMaxVersion maxVersion = FrameBuffer::getMaxGLESVersion();
            switch (maxVersion) {
            // Underlying GLES implmentation's max version string
            // is allowed to be higher than the version of the request
            // for the context---it can create a higher version context,
            // and return simply the max possible version overall.
            case GLES_DISPATCH_MAX_VERSION_2:
                glStr = replaceESVersionString(glStr, "2.0");
                break;
            case GLES_DISPATCH_MAX_VERSION_3_0:
                glStr = replaceESVersionString(glStr, "3.0");
                break;
            case GLES_DISPATCH_MAX_VERSION_3_1:
                glStr = replaceESVersionString(glStr, "3.1");
                break;
            default:
                break;
            }
        } else {
            glStr = replaceESVersionString(glStr, "2.0");
        }
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
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    int numConfigs = 0, numAttribs = 0;

    FrameBuffer::getFB()->getConfigs()->getPackInfo(&numConfigs, &numAttribs);
    if (p_numAttribs) {
        *p_numAttribs = static_cast<uint32_t>(numAttribs);
    }
    return numConfigs;
}

static EGLint rcGetConfigs(uint32_t bufSize, GLuint* buffer)
{
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    GLuint bufferSize = (GLuint)bufSize;
    return FrameBuffer::getFB()->getConfigs()->packConfigs(bufferSize, buffer);
}

static EGLint rcChooseConfig(EGLint *attribs,
                             uint32_t attribs_size,
                             uint32_t *configs,
                             uint32_t configs_size)
{
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb || attribs_size==0) {
        return 0;
    }

    return fb->getConfigs()->chooseConfig(
            attribs, (EGLint*)configs, (EGLint)configs_size);
}

static EGLint rcGetFBParam(EGLint param)
{
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
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
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return 0;
    }

    HandleType ret = fb->createRenderContext(config, share, (GLESApi)glVersion);
    return ret;
}

static void rcDestroyContext(uint32_t context)
{
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return;
    }

    fb->DestroyRenderContext(context);
}

static uint32_t rcCreateWindowSurface(uint32_t config,
                                      uint32_t width, uint32_t height)
{
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return 0;
    }

    return fb->createWindowSurface(config, width, height);
}

static void rcDestroyWindowSurface(uint32_t windowSurface)
{
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return;
    }

    fb->DestroyWindowSurface( windowSurface );
}

static uint32_t rcCreateColorBuffer(uint32_t width,
                                    uint32_t height, GLenum internalFormat)
{
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return 0;
    }

    return fb->createColorBuffer(width, height, internalFormat,
                                 FRAMEWORK_FORMAT_GL_COMPATIBLE);
}

static uint32_t rcCreateColorBufferDMA(uint32_t width,
                                       uint32_t height, GLenum internalFormat,
                                       int frameworkFormat)
{
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return 0;
    }

    return fb->createColorBuffer(width, height, internalFormat,
                                 (FrameworkFormat)frameworkFormat);
}

static int rcOpenColorBuffer2(uint32_t colorbuffer)
{
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
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
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return;
    }
    fb->closeColorBuffer( colorbuffer );
}

static int rcFlushWindowColorBuffer(uint32_t windowSurface)
{
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    GRSYNC_DPRINT("waiting for gralloc cb lock");
    GrallocSyncPostLock lock(sGrallocSync.get());
    GRSYNC_DPRINT("lock gralloc cb lock {");

    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        GRSYNC_DPRINT("unlock gralloc cb lock");
        return -1;
    }

    // Update from Vulkan if necessary
    goldfish_vk::updateColorBufferFromVkImage(
        fb->getWindowSurfaceColorBufferHandle(windowSurface));

    if (!fb->flushWindowSurfaceColorBuffer(windowSurface)) {
        GRSYNC_DPRINT("unlock gralloc cb lock }");
        return -1;
    }

    // Update to Vulkan if necessary
    goldfish_vk::updateVkImageFromColorBuffer(
        fb->getWindowSurfaceColorBufferHandle(windowSurface));

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
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return;
    }
    fb->setWindowSurfaceColorBuffer(windowSurface, colorBuffer);
}

static EGLint rcMakeCurrent(uint32_t context,
                            uint32_t drawSurf, uint32_t readSurf)
{
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return EGL_FALSE;
    }

    bool ret = fb->bindContext(context, drawSurf, readSurf);

    return (ret ? EGL_TRUE : EGL_FALSE);
}

static void rcFBPost(uint32_t colorBuffer)
{
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return;
    }

    // Update from Vulkan if necessary
    goldfish_vk::updateColorBufferFromVkImage(colorBuffer);

    fb->post(colorBuffer);
}

static void rcFBSetSwapInterval(EGLint interval)
{
   // XXX: TBD - should be implemented
}

static void rcBindTexture(uint32_t colorBuffer)
{
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return;
    }

    // Update from Vulkan if necessary
    goldfish_vk::updateColorBufferFromVkImage(colorBuffer);

    fb->bindColorBufferToTexture(colorBuffer);
}

static void rcBindRenderbuffer(uint32_t colorBuffer)
{
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return;
    }

    // Update from Vulkan if necessary
    goldfish_vk::updateColorBufferFromVkImage(colorBuffer);

    fb->bindColorBufferToRenderbuffer(colorBuffer);
}

static EGLint rcColorBufferCacheFlush(uint32_t colorBuffer,
                                      EGLint postCount, int forRead)
{
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
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
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return;
    }

    // Update from Vulkan if necessary
    goldfish_vk::updateColorBufferFromVkImage(colorBuffer);

    fb->readColorBuffer(colorBuffer, x, y, width, height, format, type, pixels);
}

static int rcUpdateColorBuffer(uint32_t colorBuffer,
                               GLint x, GLint y,
                               GLint width, GLint height,
                               GLenum format, GLenum type, void* pixels)
{
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    FrameBuffer *fb = FrameBuffer::getFB();

    if (!fb) {
        GRSYNC_DPRINT("unlock gralloc cb lock");
        sGrallocSync->unlockColorBufferPrepare();
        return -1;
    }

    // Since this is a modify operation, also read the current contents
    // of the VkImage, if any.
    goldfish_vk::updateColorBufferFromVkImage(colorBuffer);

    fb->updateColorBuffer(colorBuffer, x, y, width, height, format, type, pixels);

    GRSYNC_DPRINT("unlock gralloc cb lock");
    sGrallocSync->unlockColorBufferPrepare();

    // Update to Vulkan if necessary
    goldfish_vk::updateVkImageFromColorBuffer(colorBuffer);

    return 0;
}

static int rcUpdateColorBufferDMA(uint32_t colorBuffer,
                                  GLint x, GLint y,
                                  GLint width, GLint height,
                                  GLenum format, GLenum type,
                                  void* pixels, uint32_t pixels_size)
{
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    FrameBuffer *fb = FrameBuffer::getFB();

    if (!fb) {
        GRSYNC_DPRINT("unlock gralloc cb lock");
        sGrallocSync->unlockColorBufferPrepare();
        return -1;
    }

    // Since this is a modify operation, also read the current contents
    // of the VkImage, if any.
    goldfish_vk::updateColorBufferFromVkImage(colorBuffer);

    fb->updateColorBuffer(colorBuffer, x, y, width, height,
                          format, type, pixels);

    GRSYNC_DPRINT("unlock gralloc cb lock");
    sGrallocSync->unlockColorBufferPrepare();

    // Update to Vulkan if necessary
    goldfish_vk::updateVkImageFromColorBuffer(colorBuffer);

    return 0;
}

static uint32_t rcCreateClientImage(uint32_t context, EGLenum target, GLuint buffer)
{
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return 0;
    }

    return fb->createClientImage(context, target, buffer);
}

static int rcDestroyClientImage(uint32_t image)
{
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return 0;
    }

    return fb->destroyClientImage(image);
}

static void rcSelectChecksumHelper(uint32_t protocol, uint32_t reserved) {
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    ChecksumCalculatorThreadInfo::setVersion(protocol);
}

// |rcTriggerWait| is called from the goldfish sync
// kernel driver whenever a native fence fd is created.
// We will then need to use the host to find out
// when to signal that native fence fd. We use
// SyncThread for that.
static void rcTriggerWait(uint64_t eglsync_ptr,
                          uint64_t thread_ptr,
                          uint64_t timeline) {
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    if (thread_ptr == 1) {
        // Is vulkan sync fd;
        // just signal right away for now
        SyncThread::get()->triggerWait(0, timeline);
    }

    FenceSync* fenceSync = (FenceSync*)(uintptr_t)eglsync_ptr;
    EGLSYNC_DPRINT("eglsync=0x%llx "
                   "fenceSync=%p "
                   "thread_ptr=0x%llx "
                   "timeline=0x%llx",
                   eglsync_ptr, fenceSync, thread_ptr, timeline);
    SyncThread::get()->triggerWait(fenceSync, timeline);
}

// |rcCreateSyncKHR| implements the guest's |eglCreateSyncKHR| by calling the
// host's implementation of |eglCreateSyncKHR|. A SyncThread is also notified
// for purposes of signaling any native fence fd's that get created in the
// guest off the sync object created here.
static void rcCreateSyncKHR(EGLenum type,
                            EGLint* attribs,
                            uint32_t num_attribs,
                            int destroy_when_signaled,
                            uint64_t* eglsync_out,
                            uint64_t* syncthread_out) {
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    EGLSYNC_DPRINT("type=0x%x num_attribs=%d",
            type, num_attribs);

    bool hasNativeFence =
        type == EGL_SYNC_NATIVE_FENCE_ANDROID;

    // Usually we expect rcTriggerWait to be registered
    // at the beginning in rcGetRendererVersion, called
    // on init for all contexts.
    // But if we are loading from snapshot, that's not
    // guaranteed, and we need to make sure
    // rcTriggerWait is registered.
    emugl_sync_register_trigger_wait(rcTriggerWait);
    FenceSync* fenceSync = new FenceSync(hasNativeFence,
                                         destroy_when_signaled);

    // This MUST be present, or we get a deadlock effect.
    s_gles2.glFlush();

    if (syncthread_out) *syncthread_out =
        reinterpret_cast<uint64_t>(SyncThread::get());

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
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    RenderThreadInfo *tInfo = RenderThreadInfo::get();
    FrameBuffer *fb = FrameBuffer::getFB();

    EGLSYNC_DPRINT("handle=0x%lx flags=0x%x timeout=%" PRIu64,
                handle, flags, timeout);

    FenceSync* fenceSync = FenceSync::getFromHandle(handle);

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

static void rcWaitSyncKHR(uint64_t handle,
                                  EGLint flags) {
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    RenderThreadInfo *tInfo = RenderThreadInfo::get();
    FrameBuffer *fb = FrameBuffer::getFB();

    EGLSYNC_DPRINT("handle=0x%lx flags=0x%x", handle, flags);

    FenceSync* fenceSync = FenceSync::getFromHandle(handle);

    if (!fenceSync) { return; }

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

    fenceSync->waitAsync();
}

static int rcDestroySyncKHR(uint64_t handle) {
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    FenceSync* fenceSync = FenceSync::getFromHandle(handle);
    if (!fenceSync) return 0;
    fenceSync->decRef();
    return 0;
}

static void rcSetPuid(uint64_t puid) {
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    RenderThreadInfo *tInfo = RenderThreadInfo::get();
    tInfo->m_puid = puid;
}

static int rcCompose(uint32_t bufferSize, void* buffer) {
    AEMU_SCOPED_THRESHOLD_TRACE_CALL();
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return -1;
    }
    return fb->compose(bufferSize, buffer);
}

static int rcCreateDisplay(uint32_t* displayId) {
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return -1;
    }

    return fb->createDisplay(displayId);
}

static int rcDestroyDisplay(uint32_t displayId) {
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return -1;
    }

    return fb->destroyDisplay(displayId);
}

static int rcSetDisplayColorBuffer(uint32_t displayId, uint32_t colorBuffer) {
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return -1;
    }

    return fb->setDisplayColorBuffer(displayId, colorBuffer);
}

static int rcGetDisplayColorBuffer(uint32_t displayId, uint32_t* colorBuffer) {
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return -1;
    }

    return fb->getDisplayColorBuffer(displayId, colorBuffer);
}

static int rcGetColorBufferDisplay(uint32_t colorBuffer, uint32_t* displayId) {
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return -1;
    }

    return fb->getColorBufferDisplay(colorBuffer, displayId);
}

static int rcGetDisplayPose(uint32_t displayId, uint32_t* x, uint32_t* y, uint32_t* w, uint32_t* h) {
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return -1;
    }

    return fb->getDisplayPose(displayId, x, y, w, h);
}

static int rcSetDisplayPose(uint32_t displayId, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return -1;
    }

    return fb->setDisplayPose(displayId, x, y, w, h);
}

static int rcSetColorBufferVulkanMode(uint32_t colorBuffer, uint32_t mode) {
    if (!goldfish_vk::isColorBufferVulkanCompatible(colorBuffer)) {
        fprintf(
            stderr,
            "%s: error: colorBuffer 0x%x is not Vulkan compatible\n",
            __func__, colorBuffer);
        return -1;
    }

#define VULKAN_MODE_VULKAN_ONLY 1

    bool modeIsVulkanOnly = mode == VULKAN_MODE_VULKAN_ONLY;

    if (!goldfish_vk::setupVkColorBuffer(colorBuffer, modeIsVulkanOnly)) {
        fprintf(
            stderr,
            "%s: error: failed to create VkImage for colorBuffer 0x%x\n",
            __func__, colorBuffer);
        return -1;
    }

    if (!goldfish_vk::setColorBufferVulkanMode(colorBuffer, mode)) {
        fprintf(
            stderr,
            "%s: error: failed to set Vulkan mode for colorBuffer 0x%x\n",
            __func__, colorBuffer);
        return -1;
    }

    return 0;
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
    dec->rcUpdateColorBufferDMA = rcUpdateColorBufferDMA;
    dec->rcCreateColorBufferDMA = rcCreateColorBufferDMA;
    dec->rcWaitSyncKHR = rcWaitSyncKHR;
    dec->rcCompose = rcCompose;
    dec->rcCreateDisplay = rcCreateDisplay;
    dec->rcDestroyDisplay = rcDestroyDisplay;
    dec->rcSetDisplayColorBuffer = rcSetDisplayColorBuffer;
    dec->rcGetDisplayColorBuffer = rcGetDisplayColorBuffer;
    dec->rcGetColorBufferDisplay = rcGetColorBufferDisplay;
    dec->rcGetDisplayPose = rcGetDisplayPose;
    dec->rcSetDisplayPose = rcSetDisplayPose;
    dec->rcSetColorBufferVulkanMode = rcSetColorBufferVulkanMode;
}
