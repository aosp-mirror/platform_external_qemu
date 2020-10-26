// Copyright (C) 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "android/base/files/MemStream.h"
#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestTempDir.h"
#include "android/emulation/address_space_device.h"
#include "android/emulation/address_space_device.hpp"
#include "android/emulation/address_space_graphics.h"
#include "android/emulation/address_space_graphics_types.h"
#include "android/emulation/AndroidPipe.h"
#include "android/emulation/android_pipe_device.h"
#include "android/emulation/control/vm_operations.h"
#include "android/emulation/control/window_agent.h"
#include "android/emulation/HostmemIdMapping.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/globals.h"
#include "android/opengl/emugl_config.h"
#include "android/opengles-pipe.h"
#include "android/opengles.h"
#include "android/refcount-pipe.h"
#include "android/snapshot/interface.h"
#include "android/console.h"

#include <fstream>
#include <string>

#include <stdio.h>

#include "VulkanDispatch.h"
#include "GfxStreamAgents.h"

#define GFXSTREAM_DEBUG_LEVEL 1

#if GFXSTREAM_DEBUG_LEVEL >= 1
#define GFXS_LOG(fmt, ...)                                                     \
    do {                                                                       \
        fprintf(stdout, "%s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__); \
        fflush(stdout);                                                        \
    } while (0)

#else
#define GFXS_LOG(fmt,...)
#endif

extern "C" {
#include "hw/misc/goldfish_pipe.h"
#include "hw/virtio/virtio-goldfish-pipe.h"
}  // extern "C"

using android::AndroidPipe;
using android::base::pj;
using android::base::System;

static bool sInitialized = false;

#ifdef _WIN32
#define VG_EXPORT __declspec(dllexport)
#else
#define VG_EXPORT __attribute__((visibility("default")))
#endif

#define POST_CALLBACK_DISPLAY_TYPE_X 0
#define POST_CALLBACK_DISPLAY_TYPE_WAYLAND_SHARED_MEM 1
#define POST_CALLBACK_DISPLAY_TYPE_WINDOWS_HWND 2

struct renderer_display_info;
typedef void (*get_pixels_t)(void*, uint32_t, uint32_t);
static get_pixels_t sGetPixelsFunc = 0;
typedef void (*post_callback_t)(void*, uint32_t, int, int, int, int, int, unsigned char*);



extern "C" VG_EXPORT void gfxstream_backend_init(
    uint32_t display_width,
    uint32_t display_height,
    uint32_t display_type,
    void* renderer_cookie,
    int renderer_flags,
    struct virgl_renderer_callbacks* virglrenderer_callbacks);

extern "C" VG_EXPORT void gfxstream_backend_setup_window(
        void* native_window_handle,
        int32_t window_x,
        int32_t window_y,
        int32_t window_width,
        int32_t window_height,
        int32_t fb_width,
        int32_t fb_height);

// For reading back rendered contents to display
extern "C" VG_EXPORT void get_pixels(void* pixels, uint32_t bytes);

static const GoldfishPipeServiceOps goldfish_pipe_service_ops = {
        // guest_open()
        [](GoldfishHwPipe* hwPipe) -> GoldfishHostPipe* {
            return static_cast<GoldfishHostPipe*>(
                    android_pipe_guest_open(hwPipe));
        },
        // guest_open_with_flags()
        [](GoldfishHwPipe* hwPipe, uint32_t flags) -> GoldfishHostPipe* {
            return static_cast<GoldfishHostPipe*>(
                    android_pipe_guest_open_with_flags(hwPipe, flags));
        },
        // guest_close()
        [](GoldfishHostPipe* hostPipe, GoldfishPipeCloseReason reason) {
            static_assert((int)GOLDFISH_PIPE_CLOSE_GRACEFUL ==
                                  (int)PIPE_CLOSE_GRACEFUL,
                          "Invalid PIPE_CLOSE_GRACEFUL value");
            static_assert(
                    (int)GOLDFISH_PIPE_CLOSE_REBOOT == (int)PIPE_CLOSE_REBOOT,
                    "Invalid PIPE_CLOSE_REBOOT value");
            static_assert((int)GOLDFISH_PIPE_CLOSE_LOAD_SNAPSHOT ==
                                  (int)PIPE_CLOSE_LOAD_SNAPSHOT,
                          "Invalid PIPE_CLOSE_LOAD_SNAPSHOT value");
            static_assert(
                    (int)GOLDFISH_PIPE_CLOSE_ERROR == (int)PIPE_CLOSE_ERROR,
                    "Invalid PIPE_CLOSE_ERROR value");

            android_pipe_guest_close(hostPipe,
                                     static_cast<PipeCloseReason>(reason));
        },
        // guest_pre_load()
        [](QEMUFile* file) { (void)file; },
        // guest_post_load()
        [](QEMUFile* file) { (void)file; },
        // guest_pre_save()
        [](QEMUFile* file) { (void)file; },
        // guest_post_save()
        [](QEMUFile* file) { (void)file; },
        // guest_load()
        [](QEMUFile* file,
           GoldfishHwPipe* hwPipe,
           char* force_close) -> GoldfishHostPipe* {
            (void)file;
            (void)hwPipe;
            (void)force_close;
           return nullptr;
        },
        // guest_save()
        [](GoldfishHostPipe* hostPipe, QEMUFile* file) {
            (void)hostPipe;
            (void)file;
        },
        // guest_poll()
        [](GoldfishHostPipe* hostPipe) {
            static_assert((int)GOLDFISH_PIPE_POLL_IN == (int)PIPE_POLL_IN,
                          "invalid POLL_IN values");
            static_assert((int)GOLDFISH_PIPE_POLL_OUT == (int)PIPE_POLL_OUT,
                          "invalid POLL_OUT values");
            static_assert((int)GOLDFISH_PIPE_POLL_HUP == (int)PIPE_POLL_HUP,
                          "invalid POLL_HUP values");

            return static_cast<GoldfishPipePollFlags>(
                    android_pipe_guest_poll(hostPipe));
        },
        // guest_recv()
        [](GoldfishHostPipe* hostPipe,
           GoldfishPipeBuffer* buffers,
           int numBuffers) -> int {
            // NOTE: Assumes that AndroidPipeBuffer and GoldfishPipeBuffer
            //       have exactly the same layout.
            static_assert(
                    sizeof(AndroidPipeBuffer) == sizeof(GoldfishPipeBuffer),
                    "Invalid PipeBuffer sizes");
        // We can't use a static_assert with offsetof() because in msvc, it uses
        // reinterpret_cast.
        // TODO: Add runtime assertion instead?
        // https://developercommunity.visualstudio.com/content/problem/22196/static-assert-cannot-compile-constexprs-method-tha.html
#ifndef _MSC_VER
            static_assert(offsetof(AndroidPipeBuffer, data) ==
                                  offsetof(GoldfishPipeBuffer, data),
                          "Invalid PipeBuffer::data offsets");
            static_assert(offsetof(AndroidPipeBuffer, size) ==
                                  offsetof(GoldfishPipeBuffer, size),
                          "Invalid PipeBuffer::size offsets");
#endif
            return android_pipe_guest_recv(
                    hostPipe, reinterpret_cast<AndroidPipeBuffer*>(buffers),
                    numBuffers);
        },
        // guest_send()
        [](GoldfishHostPipe** hostPipe,
           const GoldfishPipeBuffer* buffers,
           int numBuffers) -> int {
            return android_pipe_guest_send(
                    reinterpret_cast<void**>(hostPipe),
                    reinterpret_cast<const AndroidPipeBuffer*>(buffers),
                    numBuffers);
        },
        // guest_wake_on()
        [](GoldfishHostPipe* hostPipe, GoldfishPipeWakeFlags wakeFlags) {
            android_pipe_guest_wake_on(hostPipe, static_cast<int>(wakeFlags));
        },
        // dma_add_buffer()
        [](void* pipe, uint64_t paddr, uint64_t sz) {
            // not considered for virtio
        },
        // dma_remove_buffer()
        [](uint64_t paddr) {
            // not considered for virtio
        },
        // dma_invalidate_host_mappings()
        []() {
            // not considered for virtio
        },
        // dma_reset_host_mappings()
        []() {
            // not considered for virtio
        },
        // dma_save_mappings()
        [](QEMUFile* file) {
            (void)file;
        },
        // dma_load_mappings()
        [](QEMUFile* file) {
            (void)file;
        },
};

static android::base::TestTempDir* sTestContentDir = nullptr;

extern const QAndroidVmOperations* const gQAndroidVmOperations;

static void set_post_callback(struct renderer_display_info* r, post_callback_t func, uint32_t display_type);

static void default_post_callback(
    void* context, uint32_t displayId, int width, int height, int ydir, int format, int frame_type, unsigned char* pixels) {
    (void)context;
    (void)width;
    (void)height;
    (void)ydir;
    (void)format;
    (void)frame_type;
    (void)pixels;
    // no-op
}

uint32_t sBackendFlags = 0;

enum BackendFlags {
    GFXSTREAM_BACKEND_FLAGS_NO_VK_BIT = 1 << 0,
    GFXSTREAM_BACKEND_FLAGS_EGL2EGL_BIT = 1 << 1,
};

// based on VIRGL_RENDERER_USE* and friends
enum RendererFlags {
    GFXSTREAM_RENDERER_FLAGS_USE_EGL_BIT = 1 << 0,
    GFXSTREAM_RENDERER_FLAGS_THREAD_SYNC = 1 << 1,
    GFXSTREAM_RENDERER_FLAGS_USE_GLX_BIT = 1 << 2,
    GFXSTREAM_RENDERER_FLAGS_USE_SURFACELESS_BIT = 1 << 3,
    GFXSTREAM_RENDERER_FLAGS_USE_GLES_BIT = 1 << 4,
    GFXSTREAM_RENDERER_FLAGS_NO_VK_BIT = 1 << 5, // for disabling vk
    GFXSTREAM_RENDERER_FLAGS_IGNORE_HOST_GL_ERRORS_BIT = 1 << 6, // control IgnoreHostOpenGLErrors flag
    GFXSTREAM_RENDERER_FLAGS_NATIVE_TEXTURE_DECOMPRESSION_BIT = 1 << 7, // Attempt GPU texture decompression
    GFXSTREAM_RENDERER_FLAGS_ENABLE_BPTC_TEXTURES_BIT = 1 << 8, // enable BPTC texture support if available
    GFXSTREAM_RENDERER_FLAGS_ENABLE_GLES31_BIT = 1 << 9, // disables the PlayStoreImage flag

    GFXSTREAM_RENDERER_FLAGS_NO_SYNCFD_BIT = 1 << 20, // for disabling syncfd
    GFXSTREAM_RENDERER_FLAGS_GUEST_USES_ANGLE = 1 << 21,
};

// Sets backend flags for different kinds of initialization.
// Default (and default if not called): flags == 0
// Needs to be called before |gfxstream_backend_init|.
extern "C" VG_EXPORT void gfxstream_backend_set_flags(uint32_t flags) {
    sBackendFlags = flags;
}

extern "C" VG_EXPORT void gfxstream_backend_init(
    uint32_t display_width,
    uint32_t display_height,
    uint32_t display_type,
    void* renderer_cookie,
    int renderer_flags,
    struct virgl_renderer_callbacks* virglrenderer_callbacks) {


    // First we make some agents available.


    GFXS_LOG("start. display dimensions: width %u height %u. backend flags: 0x%x renderer flags: 0x%x",
             display_width, display_height, sBackendFlags, renderer_flags);

    AvdInfo** avdInfoPtr = aemu_get_android_avdInfoPtr();

    (*avdInfoPtr) = avdInfo_newCustom(
        "goldfish_opengl_test",
        28,
        "x86_64",
        "x86_64",
        true /* is google APIs */,
        AVD_PHONE);

    // Flags processing

    // TODO: hook up "gfxstream egl" to the renderer flags
    // GFXSTREAM_RENDERER_FLAGS_USE_EGL_BIT in crosvm
    // as it's specified from launch_cvd.
    // At the moment, use ANDROID_GFXSTREAM_EGL=1
    // For test on GCE
    if (System::getEnvironmentVariable("ANDROID_GFXSTREAM_EGL") == "1") {
        System::setEnvironmentVariable("ANDROID_EGL_ON_EGL", "1");
        System::setEnvironmentVariable("ANDROID_EMUGL_LOG_PRINT", "1");
        System::setEnvironmentVariable("ANDROID_EMUGL_VERBOSE", "1");
    }
    // end for test on GCE

    System::setEnvironmentVariable("ANDROID_EMU_HEADLESS", "1");
    System::setEnvironmentVariable("ANDROID_EMU_SANDBOX", "1");
    System::setEnvironmentVariable("ANDROID_EMUGL_FIXED_BACKEND_LIST", "1");
    bool vkDisabledByEnv = System::getEnvironmentVariable("ANDROID_EMU_DISABLE_VULKAN") == "1";
    bool vkDisabledByFlag =
        (sBackendFlags & GFXSTREAM_BACKEND_FLAGS_NO_VK_BIT) ||
        (renderer_flags & GFXSTREAM_RENDERER_FLAGS_NO_VK_BIT);
    bool enableVk = !vkDisabledByEnv && !vkDisabledByFlag;

    bool egl2eglByEnv = System::getEnvironmentVariable("ANDROID_EGL_ON_EGL") == "1";
    bool egl2eglByFlag = renderer_flags & GFXSTREAM_RENDERER_FLAGS_USE_EGL_BIT;
    bool enable_egl2egl = egl2eglByFlag || egl2eglByEnv;
    if (enable_egl2egl) {
        System::setEnvironmentVariable("ANDROID_GFXSTREAM_EGL", "1");
        System::setEnvironmentVariable("ANDROID_EGL_ON_EGL", "1");
    }

    bool ignoreHostGlErrorsFlag = renderer_flags & GFXSTREAM_RENDERER_FLAGS_IGNORE_HOST_GL_ERRORS_BIT;
    bool nativeTextureDecompression = renderer_flags & GFXSTREAM_RENDERER_FLAGS_NATIVE_TEXTURE_DECOMPRESSION_BIT;
    bool bptcTextureSupport = renderer_flags & GFXSTREAM_RENDERER_FLAGS_ENABLE_BPTC_TEXTURES_BIT;
    bool syncFdDisabledByFlag = renderer_flags & GFXSTREAM_RENDERER_FLAGS_NO_SYNCFD_BIT;
    bool surfaceless =
            renderer_flags & GFXSTREAM_RENDERER_FLAGS_USE_SURFACELESS_BIT;
    bool enableGlEs31Flag = renderer_flags & GFXSTREAM_RENDERER_FLAGS_ENABLE_GLES31_BIT;
    bool guestUsesAngle = renderer_flags & GFXSTREAM_RENDERER_FLAGS_GUEST_USES_ANGLE;

    GFXS_LOG("Vulkan enabled? %d", enableVk);
    GFXS_LOG("egl2egl enabled? %d", enable_egl2egl);
    GFXS_LOG("ignore host gl errors enabled? %d", ignoreHostGlErrorsFlag);
    GFXS_LOG("syncfd enabled? %d", !syncFdDisabledByFlag);
    GFXS_LOG("use native texture decompression if available? %d", nativeTextureDecompression);
    GFXS_LOG("enable BPTC support if available? %d", bptcTextureSupport);
    GFXS_LOG("surfaceless? %d", surfaceless);
    GFXS_LOG("OpenGL ES 3.1 enabled? %d", enableGlEs31Flag);
    GFXS_LOG("guest using ANGLE? %d", guestUsesAngle);

    // Need to manually set the GLES backend paths in gfxstream environment
    // because the library search paths are not automatically set to include
    // the directory in whioch the GLES backend resides.
#if defined(__linux__)
#define GFXSTREAM_LIB_SUFFIX ".so"
#elif defined(__APPLE__)
#define GFXSTREAM_LIB_SUFFIX ".dylib"
#else // Windows
#define GFXSTREAM_LIB_SUFFIX ".dll"
#endif

    auto dispString = System::get()->envGet("DISPLAY");
    GFXS_LOG("current display: %s", dispString.c_str());

    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::GLPipeChecksum, false);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::GLESDynamicVersion, true);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::PlayStoreImage, !enableGlEs31Flag);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::GLDMA, false);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::GLAsyncSwap, false);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::RefCountPipe, true);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::IgnoreHostOpenGLErrors, ignoreHostGlErrorsFlag);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::NativeTextureDecompression, nativeTextureDecompression);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::BptcTextureSupport, bptcTextureSupport);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::GLDirectMem, false);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::Vulkan, enableVk);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::VulkanSnapshots, false);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::VulkanNullOptionalStrings, true);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::HostComposition, true);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::VulkanIgnoredHandles, true);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::VirtioGpuNext, true);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::VirtioGpuNativeSync, !syncFdDisabledByFlag);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::GuestUsesAngle, guestUsesAngle);

    emugl::vkDispatch(false /* don't use test ICD */);

    auto androidHw = aemu_get_android_hw();

    androidHw->hw_gltransport_asg_writeBufferSize = 262144;
    androidHw->hw_gltransport_asg_writeStepSize = 8192;
    androidHw->hw_gltransport_asg_dataRingSize = 131072;
    androidHw->hw_gltransport_drawFlushInterval = 800;

    EmuglConfig config;

    // Make all the console agents available.
    android::emulation::injectConsoleAgents(android::emulation::GfxStreamAndroidConsoleFactory());

    emuglConfig_init(&config, true /* gpu enabled */, "auto",
                     enable_egl2egl ? "swiftshader_indirect" : "host",
                     64,          /* bitness */
                     surfaceless, /* no window */
                     false,       /* blacklisted */
                     false,       /* has guest renderer */
                     WINSYS_GLESBACKEND_PREFERENCE_AUTO,
                     false /* force host gpu vulkan */);

    emuglConfig_setupEnv(&config);

    android_initOpenglesEmulation();
    int maj;
    int min;
    android_startOpenglesRenderer(
        display_width, display_height, 1, 28,
        getConsoleAgents()->vm,
        getConsoleAgents()->emu,
        getConsoleAgents()->multi_display,
        &maj, &min);

    char* vendor = nullptr;
    char* renderer = nullptr;
    char* version = nullptr;

    android_getOpenglesHardwareStrings(
        &vendor, &renderer, &version);

    GFXS_LOG("GL strings; [%s] [%s] [%s].\n",
             vendor, renderer, version);

    auto openglesRenderer = android_getOpenglesRenderer().get();

    if (!openglesRenderer) {
        fprintf(stderr, "%s: no renderer started, fatal\n", __func__);
        abort();
    }

    address_space_set_vm_operations(getConsoleAgents()->vm);
    android_init_opengles_pipe();
    android_opengles_pipe_set_recv_mode(2 /* virtio-gpu */);
    android_init_refcount_pipe();

    sGetPixelsFunc = android_getReadPixelsFunc();

    pipe_virgl_renderer_init(renderer_cookie, renderer_flags, virglrenderer_callbacks);

    GFXS_LOG("Started renderer");

    if (surfaceless) {
        set_post_callback(nullptr, default_post_callback, display_type);
    }
}

extern "C" VG_EXPORT void gfxstream_backend_setup_window(
        void* native_window_handle,
        int32_t window_x,
        int32_t window_y,
        int32_t window_width,
        int32_t window_height,
        int32_t fb_width,
        int32_t fb_height) {
    android_showOpenglesWindow(native_window_handle, window_x, window_y,
                               window_width, window_height, fb_width, fb_height,
                               1.0f, 0, false, false);
}

static void set_post_callback(struct renderer_display_info* r, post_callback_t func, uint32_t display_type) {

    // crosvm needs bgra readback depending on the display type
    bool use_bgra_readback = false;
    switch (display_type) {
        case POST_CALLBACK_DISPLAY_TYPE_X:
            GFXS_LOG("using display type: X11");
            use_bgra_readback = true;
            break;
        case POST_CALLBACK_DISPLAY_TYPE_WAYLAND_SHARED_MEM:
            GFXS_LOG("using display type: wayland shared mem");
            break;
        case POST_CALLBACK_DISPLAY_TYPE_WINDOWS_HWND:
            GFXS_LOG("using display type: windows hwnd");
            break;
        default:
            break;
    }

    android_setPostCallback(func, r, false, 0);
}

extern "C" VG_EXPORT void gfxstream_backend_teardown() {
    android_finishOpenglesRenderer();
    android_hideOpenglesWindow();
    android_stopOpenglesRenderer(true);
}

extern "C" VG_EXPORT void gfxstream_backend_set_screen_mask(int width, int height, const unsigned char* rgbaData) {
    android_setOpenglesScreenMask(width, height, rgbaData);
}

extern "C" VG_EXPORT void get_pixels(void* pixels, uint32_t bytes) {
    //TODO: support display > 0
    sGetPixelsFunc(pixels, bytes, 0);
}

extern "C" const GoldfishPipeServiceOps* goldfish_pipe_get_service_ops() {
    return &goldfish_pipe_service_ops;
}

