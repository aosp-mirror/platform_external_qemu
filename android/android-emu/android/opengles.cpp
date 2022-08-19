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

#include "android/base/CpuUsage.h"
#include "android/base/GLObjectCounter.h"
#include "android/base/files/PathUtils.h"
#include "android/base/files/Stream.h"
#include "android/base/memory/MemoryTracker.h"
#include "android/base/system/System.h"
#include "android/crashreport/crash-handler.h"
#include "android/emulation/address_space_device.h"
#include "android/emulation/address_space_graphics.h"
#include "android/emulation/address_space_graphics_types.h"
#include "android/emulation/GoldfishDma.h"
#include "android/emulation/RefcountPipe.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/console.h"
#include "android/opengl/emugl_config.h"
#include "android/opengl/logger.h"
#include "android/opengl/GLProcessPipe.h"
#include "android/snapshot/PathUtils.h"
#include "android/snapshot/Snapshotter.h"
#include "android/utils/bufprint.h"
#include "android/utils/debug.h"
#include "android/utils/dll.h"
#include "android/utils/GfxstreamFatalError.h"
#include "android/utils/path.h"
#include "config-host.h"

#include "OpenglRender/render_api_functions.h"
#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define D(...) do { \
    VERBOSE_PRINT(init,__VA_ARGS__); \
    android_opengl_logger_write(__VA_ARGS__); \
} while(0);

#define DD(...) do { \
    VERBOSE_PRINT(gles,__VA_ARGS__); \
    android_opengl_logger_write(__VA_ARGS__); \
} while(0);

#define E(fmt,...) do { \
    derror(fmt, ##__VA_ARGS__); \
    android_opengl_logger_write(fmt "\n", ##__VA_ARGS__); \
} while(0);

using android::base::pj;
using android::base::System;
using android::emulation::asg::AddressSpaceGraphicsContext;
using android::emulation::asg::ConsumerInterface;
using android::emulation::asg::ConsumerCallbacks;

/* Name of the GLES rendering library we're going to use */
#ifdef AEMU_GFXSTREAM_BACKEND
#define RENDERER_LIB_NAME "libgfxstream_backend"
#else
#define RENDERER_LIB_NAME "libOpenglRender"
#endif  // AEMU_GFXSTREAM_BACKEND

/* Declared in "android/console.h" */
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
        using type = ret(sig); \
        name = (type*)symbol; \
    } else { \
        E("GLES emulation: Could not find required symbol (%s): %s", #name, error); \
        free(error); \
        return -1; \
    }
    LIST_RENDER_API_FUNCTIONS(FUNCTION_)
#undef FUNCTION_

    return 0;
}

static bool sOpenglLoggerInitialized = false;
static bool sRendererUsesSubWindow = false;
static bool sEgl2egl = false;
static emugl::RenderLibPtr sRenderLib = nullptr;
static emugl::RendererPtr sRenderer = nullptr;

static const EGLDispatch* sEgl = nullptr;
static const GLESv2Dispatch* sGlesv2 = nullptr;

#ifdef AEMU_GFXSTREAM_BACKEND
static bool sRunningInGfxstreamBackend = false;
// The two functions below only are called in gfxstream backend, which assumes
// RenderLib is a static library.
int android_prepareOpenglesEmulation(void) {
    sRunningInGfxstreamBackend = true;
    return android_initOpenglesEmulation();
}

int android_setOpenglesEmulation(void* renderLib, void* eglDispatch, void* glesv2Dispatch) {
    sRunningInGfxstreamBackend = true;
    return 0;
}
#endif  // AEMU_GFXSTREAM_BACKEND

int android_initOpenglesEmulation() {
    android_init_opengl_logger();

    bool glFineLogging = System::get()->envGet("ANDROID_EMUGL_FINE_LOG") == "1";
    bool glLogPrinting = System::get()->envGet("ANDROID_EMUGL_LOG_PRINT") == "1";

    AndroidOpenglLoggerFlags loggerFlags =
        static_cast<AndroidOpenglLoggerFlags>(
        (glFineLogging ? OPENGL_LOGGER_DO_FINE_LOGGING : 0) |
        (glLogPrinting ? OPENGL_LOGGER_PRINT_TO_STDOUT : 0));

    android_opengl_logger_set_flags(loggerFlags);

    sOpenglLoggerInitialized = true;

    char* error = NULL;

    if (sRenderLib != NULL)
        return 0;

    D("Initializing hardware OpenGLES emulation support");

    ADynamicLibrary* rendererSo =
            adynamicLibrary_open(RENDERER_LIB_NAME, &error);
    if (rendererSo == NULL) {
        E("Could not load OpenGLES emulation library [%s]: %s",
               RENDERER_LIB_NAME, error);

        E("Retrying in program directory/lib64...");

        auto progDir = System::get()->getProgramDirectory();

        auto retryLibPath =
            pj({progDir, "lib64", RENDERER_LIB_NAME});

        rendererSo = adynamicLibrary_open(retryLibPath.c_str(), &error);

        if (rendererSo == nullptr) {
            E("Could not load OpenGLES emulation library [%s]: %s (2nd try)",
                   retryLibPath.c_str(), error);
            return -1;
        }
    }

    /* Resolve the functions */
    if (initOpenglesEmulationFuncs(rendererSo) < 0) {
        E("OpenGLES emulation library mismatch. Be sure to use the correct version!");
        crashhandler_append_message_format(
            "OpenGLES emulation library mismatch. Be sure to use the correct version!");
        goto BAD_EXIT;
    }

    sRenderLib = initLibrary();
    if (!sRenderLib) {
        E("OpenGLES initialization failed!");
        crashhandler_append_message_format("OpenGLES initialization failed!");
        goto BAD_EXIT;
    }

    sRendererUsesSubWindow = true;
    if (const char* env = getenv("ANDROID_GL_SOFTWARE_RENDERER")) {
        if (env[0] != '\0' && env[0] != '0') {
            sRendererUsesSubWindow = false;
        }
    }

    sEgl2egl = false;
    if (const char* env = getenv("ANDROID_EGL_ON_EGL")) {
        if (env[0] != '\0' && env[0] == '1') {
            sEgl2egl = true;
        }
    }

    sEgl = (const EGLDispatch *)sRenderLib->getEGLDispatch();
    sGlesv2 = (const GLESv2Dispatch *)sRenderLib->getGLESv2Dispatch();

    return 0;

BAD_EXIT:
    E("OpenGLES emulation library could not be initialized!");
    adynamicLibrary_close(rendererSo);
    return -1;
}

int
android_startOpenglesRenderer(int width, int height, bool guestPhoneApi, int guestApiLevel,
                              const QAndroidVmOperations *vm_operations,
                              const QAndroidEmulatorWindowAgent *window_agent,
                              const QAndroidMultiDisplayAgent *multi_display_agent,
                              int* glesMajorVersion_out,
                              int* glesMinorVersion_out)
{
    if (!sRenderLib) {
        D("Can't start OpenGLES renderer without support libraries");
        return -1;
    }

    if (!sEgl) {
        D("Can't start OpenGLES renderer without EGL libraries");
        return -1;
    }

    if (!sGlesv2) {
        D("Can't start OpenGLES renderer without GLES libraries");
        return -1;
    }

    if (sRenderer) {
        return 0;
    }

    const GpuInfoList& gpuList = globalGpuInfoList();
    std::string gpuInfoAsString = gpuList.dump();
    android_opengl_logger_write("%s: gpu info", __func__);
    android_opengl_logger_write("%s", gpuInfoAsString.c_str());

    sRenderLib->setRenderer(emuglConfig_get_current_renderer());
    sRenderLib->setAvdInfo(guestPhoneApi, guestApiLevel);
    sRenderLib->setCrashReporter(&crashhandler_die_format);
#if defined(AEMU_GFXSTREAM_BACKEND)
    // Don't override the feature controller when running under
    // gfxstream_backend_unittests
    if (!sRunningInGfxstreamBackend) {
        sRenderLib->setFeatureController(&android::featurecontrol::isEnabled);
    }
#else
    sRenderLib->setFeatureController(&android::featurecontrol::isEnabled);
#endif
    sRenderLib->setSyncDevice(goldfish_sync_create_timeline,
            goldfish_sync_create_fence,
            goldfish_sync_timeline_inc,
            goldfish_sync_destroy_timeline,
            goldfish_sync_register_trigger_wait,
            goldfish_sync_device_exists);
#if defined(AEMU_GFXSTREAM_BACKEND)
    sRenderLib->setGrallocImplementation(MINIGBM);
#else
    sRenderLib->setGrallocImplementation(
            android::featurecontrol::isEnabled(
                    android::featurecontrol::Minigbm) ||
                            getConsoleAgents()->settings->is_fuchsia()
                    ? MINIGBM
                    : GOLDFISH_GRALLOC);
#endif

    emugl_logger_struct logfuncs;
    logfuncs.coarse = android_opengl_logger_write;
    logfuncs.fine = android_opengl_cxt_logger_write;
    sRenderLib->setLogger(logfuncs);
    sRenderLib->setGLObjectCounter(android::base::GLObjectCounter::get());
    emugl_dma_ops dma_ops;
    dma_ops.get_host_addr = android_goldfish_dma_ops.get_host_addr;
    dma_ops.unlock = android_goldfish_dma_ops.unlock;
    sRenderLib->setDmaOps(dma_ops);
    sRenderLib->setVmOps(*vm_operations);
    sRenderLib->setAddressSpaceDeviceControlOps(get_address_space_device_control_ops());
    sRenderLib->setWindowOps(*window_agent, *multi_display_agent);
    sRenderLib->setUsageTracker(android::base::CpuUsage::get(),
                                android::base::MemoryTracker::get());

    sRenderer = sRenderLib->initRenderer(width, height, sRendererUsesSubWindow, sEgl2egl);

    android::snapshot::Snapshotter::get().addOperationCallback(
            [](android::snapshot::Snapshotter::Operation op,
               android::snapshot::Snapshotter::Stage stage) {
                sRenderer->snapshotOperationCallback(op, stage);
            });

    android::emulation::registerOnLastRefCallback(
            sRenderLib->getOnLastColorBufferRef());

    ConsumerInterface interface = {
        // create
        [](struct asg_context context,
           android::base::Stream* loadStream,
           ConsumerCallbacks callbacks) {
           return sRenderer->addressSpaceGraphicsConsumerCreate(
               context, loadStream, callbacks);
        },
        // destroy
        [](void* consumer) {
           sRenderer->addressSpaceGraphicsConsumerDestroy(consumer);
        },
        // pre save
        [](void* consumer) {
           sRenderer->addressSpaceGraphicsConsumerPreSave(consumer);
        },
        // global presave
        []() {
           sRenderer->pauseAllPreSave();
        },
        // save
        [](void* consumer, android::base::Stream* stream) {
           sRenderer->addressSpaceGraphicsConsumerSave(consumer, stream);
        },
        // global postsave
        []() {
           sRenderer->resumeAll();
        },
        // postSave
        [](void* consumer) {
           sRenderer->addressSpaceGraphicsConsumerPostSave(consumer);
        },
        // postLoad
        [](void* consumer) {
           sRenderer->addressSpaceGraphicsConsumerRegisterPostLoadRenderThread(consumer);
        },
        // global preload
        []() {
            // This wants to address that when using asg, pipe wants to clean
            // up all render threads and wait for gl objects, but framebuffer
            // notices that there is a render thread info that is still not
            // cleaned up because these render threads come from asg.
            android::opengl::forEachProcessPipeIdRunAndErase([](uint64_t id) {
                android_cleanupProcGLObjects(id);
            });
            android_waitForOpenglesProcessCleanup();
        },
    };
    AddressSpaceGraphicsContext::setConsumer(interface);

    if (!sRenderer) {
        D("Can't start OpenGLES renderer?");
        return -1;
    }

    // after initRenderer is a success, the maximum GLES API is calculated depending
    // on feature control and host GPU support. Set the obtained GLES version here.
    if (glesMajorVersion_out && glesMinorVersion_out)
        sRenderLib->getGlesVersion(glesMajorVersion_out, glesMinorVersion_out);
    return 0;
}

bool
android_asyncReadbackSupported() {
    if (sRenderer) {
        return sRenderer->asyncReadbackSupported();
    } else {
        D("tried to query async readback support "
          "before renderer initialized. Likely guest rendering");
        return false;
    }
}

void
android_setPostCallback(OnPostFunc onPost, void* onPostContext, bool useBgraReadback, uint32_t displayId)
{
    if (sRenderer) {
        sRenderer->setPostCallback(onPost, onPostContext, useBgraReadback, displayId);
    }
}

ReadPixelsFunc android_getReadPixelsFunc() {
    if (sRenderer) {
        return sRenderer->getReadPixelsCallback();
    } else {
        return nullptr;
    }
}

FlushReadPixelPipeline android_getFlushReadPixelPipeline() {
    if (sRenderer) {
        return sRenderer->getFlushReadPixelPipeline();
    } else {
        return nullptr;
    }
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
    assert(vendor != NULL && renderer != NULL && version != NULL);
    assert(*vendor == NULL && *renderer == NULL && *version == NULL);
    if (!sRenderer) {
        D("Can't get OpenGL ES hardware strings when renderer not started");
        return;
    }

    const emugl::Renderer::HardwareStrings strings =
            sRenderer->getHardwareStrings();
    D("OpenGL Vendor=[%s]", strings.vendor.c_str());
    D("OpenGL Renderer=[%s]", strings.renderer.c_str());
    D("OpenGL Version=[%s]", strings.version.c_str());

    /* Special case for the default ES to GL translators: extract the strings
     * of the underlying OpenGL implementation. */
    if (strncmp(strings.vendor.c_str(), "Google", 6) == 0 &&
            strncmp(strings.renderer.c_str(), "Android Emulator OpenGL ES Translator", 37) == 0) {
        *vendor = strdupBaseString(strings.vendor.c_str());
        *renderer = strdupBaseString(strings.renderer.c_str());
        *version = strdupBaseString(strings.version.c_str());
    } else {
        *vendor = strdup(strings.vendor.c_str());
        *renderer = strdup(strings.renderer.c_str());
        *version = strdup(strings.version.c_str());
    }
}

void android_getOpenglesVersion(int* maj, int* min) {
    sRenderLib->getGlesVersion(maj, min);
    dinfo("%s: maj min %d %d", __func__, *maj, *min);
}

void
android_stopOpenglesRenderer(bool wait)
{
    if (sRenderer) {
        sRenderer->stop(wait);
        if (wait) {
            sRenderer.reset();
            android_stop_opengl_logger();
        }
    }
}

void
android_finishOpenglesRenderer()
{
    if (sRenderer) {
        sRenderer->finish();
    }
}

static emugl::RenderOpt sOpt;
static int sWidth, sHeight;
static int sNewWidth, sNewHeight;

int android_showOpenglesWindow(void* window,
                               int wx,
                               int wy,
                               int ww,
                               int wh,
                               int fbw,
                               int fbh,
                               float dpr,
                               float rotation,
                               bool deleteExisting,
                               bool hideWindow) {
    if (!sRenderer) {
        return -1;
    }
    FBNativeWindowType win = (FBNativeWindowType)(uintptr_t)window;
    bool success = sRenderer->showOpenGLSubwindow(win, wx, wy, ww, wh, fbw, fbh,
                                                  dpr, rotation, deleteExisting,
                                                  hideWindow);
    sNewWidth = ww * dpr;
    sNewHeight = wh * dpr;
    return success ? 0 : -1;
}

void
android_setOpenglesTranslation(float px, float py)
{
    if (sRenderer) {
        sRenderer->setOpenGLDisplayTranslation(px, py);
    }
}

void
android_setOpenglesScreenMask(int width, int height, const unsigned char* rgbaData)
{
    if (sRenderer) {
        sRenderer->setScreenMask(width, height, rgbaData);
    }
}

int
android_hideOpenglesWindow(void)
{
    if (!sRenderer) {
        return -1;
    }
    bool success = sRenderer->destroyOpenGLSubwindow();
    return success ? 0 : -1;
}

void
android_redrawOpenglesWindow(void)
{
    if (sRenderer) {
        sRenderer->repaintOpenGLDisplay();
    }
}

bool
android_hasGuestPostedAFrame(void)
{
    if (sRenderer) {
        return sRenderer->hasGuestPostedAFrame();
    }
    return false;
}

void
android_resetGuestPostedAFrame(void)
{
    if (sRenderer) {
        sRenderer->resetGuestPostedAFrame();
    }
}

static ScreenshotFunc sScreenshotFunc = nullptr;

void android_registerScreenshotFunc(ScreenshotFunc f)
{
    sScreenshotFunc = f;
}

bool android_screenShot(const char* dirname, uint32_t displayId)
{
    if (sScreenshotFunc) {
        return sScreenshotFunc(dirname, displayId);
    }
    return false;
}

const emugl::RendererPtr& android_getOpenglesRenderer() {
    return sRenderer;
}

void android_cleanupProcGLObjects(uint64_t puid) {
    if (sRenderer) {
        sRenderer->cleanupProcGLObjects(puid);
    }
}

void android_cleanupProcGLObjectsAndWaitFinished(uint64_t puid) {
    if (sRenderer) {
        sRenderer->cleanupProcGLObjects(puid);
    }
}

void android_waitForOpenglesProcessCleanup() {
    if (sRenderer) {
        sRenderer->waitForProcessCleanup();
    }
}

static void* sContext, * sRenderContext, * sSurface;
static EGLint s_gles_attr[5];

extern void tinyepoxy_init(const GLESv2Dispatch* gles, int version);

static bool prepare_epoxy(void) {
    if (!sRenderLib->getOpt(&sOpt)) {
        return false;
    }
    int major, minor;
    sRenderLib->getGlesVersion(&major, &minor);
    EGLint attr[] = {
        EGL_CONTEXT_CLIENT_VERSION, major,
        EGL_CONTEXT_MINOR_VERSION_KHR, minor,
        EGL_NONE
    };
    sContext = sEgl->eglCreateContext(sOpt.display, sOpt.config, EGL_NO_CONTEXT,
                                      attr);
    if (sContext == nullptr) {
        return false;
    }
    sRenderContext = sEgl->eglCreateContext(sOpt.display, sOpt.config,
                                            sContext, attr);
    if (sRenderContext == nullptr) {
        return false;
    }
    static constexpr EGLint surface_attr[] = {
        EGL_WIDTH, 1,
        EGL_HEIGHT, 1,
        EGL_NONE
    };
    sSurface = sEgl->eglCreatePbufferSurface(sOpt.display, sOpt.config,
                                             surface_attr);
    if (sSurface == EGL_NO_SURFACE) {
        return false;
    }
    static_assert(sizeof(attr) == sizeof(s_gles_attr), "Mismatch");
    memcpy(s_gles_attr, attr, sizeof(s_gles_attr));
    tinyepoxy_init(sGlesv2, major * 10 + minor);
    return true;
}

struct DisplayChangeListener;
struct QEMUGLParams;

void * android_gl_create_context(DisplayChangeListener * unuse1,
                                 QEMUGLParams* unuse2) {
    static bool ok =  prepare_epoxy();
    if (!ok) {
        return nullptr;
    }
    sEgl->eglMakeCurrent(sOpt.display, sSurface, sSurface, sContext);
    return sEgl->eglCreateContext(sOpt.display, sOpt.config, sContext, s_gles_attr);
}

void android_gl_destroy_context(DisplayChangeListener* unused, void * ctx) {
    sEgl->eglDestroyContext(sOpt.display, ctx);
}

int android_gl_make_context_current(DisplayChangeListener* unused, void * ctx) {
    return sEgl->eglMakeCurrent(sOpt.display, sSurface, sSurface, ctx);
}

static GLuint s_tex_id, s_fbo_id;
static uint32_t s_gfx_h, s_gfx_w;
static bool s_y0_top;

// ui/gtk-egl.c:gd_egl_scanout_texture as reference.
void android_gl_scanout_texture(DisplayChangeListener* unuse,
                                uint32_t backing_id,
                                bool backing_y_0_top,
                                uint32_t backing_width,
                                uint32_t backing_height,
                                uint32_t x, uint32_t y,
                                uint32_t w, uint32_t h) {
    s_tex_id = backing_id;
    s_gfx_h = h;
    s_gfx_w = w;
    s_y0_top = backing_y_0_top;
    if (sNewWidth != sWidth || sNewHeight != sHeight) {
        sRenderLib->getOpt(&sOpt);
        sWidth = sNewWidth;
        sHeight = sNewHeight;
    }
    sEgl->eglMakeCurrent(sOpt.display, sOpt.surface, sOpt.surface,
                         sRenderContext);
    if (!s_fbo_id) {
        sGlesv2->glGenFramebuffers(1, &s_fbo_id);
    }
    sGlesv2->glBindFramebuffer(GL_FRAMEBUFFER_EXT, s_fbo_id);
    sGlesv2->glViewport(0, 0, h, w);
    sGlesv2->glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0,
                                  GL_TEXTURE_2D, backing_id, 0);
}

// ui/gtk-egl.c:gd_egl_scanout_flush as reference.
void android_gl_scanout_flush(DisplayChangeListener* unuse,
                              uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    if (!s_fbo_id)  {
        return;
    }
    sEgl->eglMakeCurrent(sOpt.display, sOpt.surface, sOpt.surface,
                         sRenderContext);

    sGlesv2->glBindFramebuffer(GL_READ_FRAMEBUFFER, s_fbo_id);
    sGlesv2->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    int y1 = s_y0_top ? 0 : s_gfx_h;
    int y2 = s_y0_top ? s_gfx_h : 0;

    sGlesv2->glViewport(0, 0, sWidth, sHeight);
    sGlesv2->glBlitFramebuffer(0, y1, s_gfx_w, y2,
                             0, 0, sWidth, sHeight,
                             GL_COLOR_BUFFER_BIT, GL_NEAREST);
    sEgl->eglSwapBuffers(sOpt.display, sOpt.surface);
    sGlesv2->glBindFramebuffer(GL_FRAMEBUFFER_EXT, s_fbo_id);
}

struct AndroidVirtioGpuOps* android_getVirtioGpuOps() {
    if (sRenderer) {
        return sRenderer->getVirtioGpuOps();
    }
    return nullptr;
}

const void* android_getEGLDispatch() {
    return sEgl;
}

const void* android_getGLESv2Dispatch() {
    return sGlesv2;
}

void android_setVsyncHz(int vsyncHz) {
    if (sRenderer) {
        sRenderer->setVsyncHz(vsyncHz);
    }
}

void android_setOpenglesDisplayConfigs(int configId, int w, int h, int dpiX,
                                       int dpiY) {
    if (sRenderer) {
        sRenderer->setDisplayConfigs(configId, w, h, dpiX, dpiY);
    }
}

void android_setOpenglesDisplayActiveConfig(int configId) {
    if (sRenderer) {
        sRenderer->setDisplayActiveConfig(configId);
    }
}
