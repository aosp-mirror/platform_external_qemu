/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include "EglOsApi.h"

#include "android/base/system/System.h"
#include "GLcommon/GLLibrary.h"
#include "OpenglCodecCommon/ErrorLog.h"
#include "emugl/common/lazy_instance.h"
#include "emugl/common/misc.h"
#include "emugl/common/shared_library.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglext_angle.h>
#include <GLES2/gl2.h>
#include <memory>

#define DEBUG 0
#if DEBUG
#define D(...) fprintf(stderr, __VA_ARGS__);
#define CHECK_EGL_ERR                                                 \
    {                                                                 \
        EGLint err = mDispatcher.eglGetError();                       \
        if (err != EGL_SUCCESS)                                       \
            D("%s: %s %d get egl error %d\n", __FUNCTION__, __FILE__, \
              __LINE__, err);                                         \
    }
#else
#define D(...) ((void)0);
#define CHECK_EGL_ERR ((void)0);
#endif

#if defined(__WIN32) || defined(_MSC_VER)

static const char* kEGLLibName = "libEGL.dll";
static const char* kGLES2LibName = "libGLESv2.dll";

#elif defined(__linux__)


static const char* kEGLLibName = "libEGL.so";
static const char* kGLES2LibName = "libGLESv2.so";

static const char* kEGLLibNameAlt = "libEGL.so.1";
static const char* kGLES2LibNameAlt = "libGLESv2.so.2";

#else // __APPLE__

#include "MacNative.h"

static const char* kEGLLibName = "libEGL.dylib";
static const char* kGLES2LibName = "libGLESv2.dylib";

#endif // __APPLE__

// List of EGL functions of interest to probe with GetProcAddress()
#define LIST_EGL_FUNCTIONS(X)                                                  \
    X(EGLBoolean, eglGetProcAddress,                                           \
      (const char* procname))                                                  \
    X(const char*, eglQueryString,                                             \
      (EGLDisplay dpy, EGLint id))                                             \
    X(EGLDisplay, eglGetPlatformDisplay,                                    \
      (EGLenum platform, void *native_display, const EGLint *attrib_list))     \
    X(EGLDisplay, eglGetPlatformDisplayEXT,                                    \
      (EGLenum platform, void *native_display, const EGLint *attrib_list))     \
    X(EGLBoolean, eglBindAPI,                                    \
      (EGLenum api)) \
    X(EGLBoolean, eglChooseConfig,                                             \
      (EGLDisplay display, EGLint const* attrib_list, EGLConfig* configs,      \
       EGLint config_size, EGLint* num_config))                                \
    X(EGLContext, eglCreateContext,                                            \
      (EGLDisplay display, EGLConfig config, EGLContext share_context,         \
       EGLint const* attrib_list))                                             \
    X(EGLSurface, eglCreatePbufferSurface,                                     \
      (EGLDisplay display, EGLConfig config, EGLint const* attrib_list))       \
    X(EGLBoolean, eglDestroyContext, (EGLDisplay display, EGLContext context)) \
    X(EGLBoolean, eglDestroySurface, (EGLDisplay display, EGLSurface surface)) \
    X(EGLBoolean, eglGetConfigAttrib,                                          \
      (EGLDisplay display, EGLConfig config, EGLint attribute,                 \
       EGLint * value))                                                        \
    X(EGLDisplay, eglGetDisplay, (NativeDisplayType native_display))           \
    X(EGLint, eglGetError, (void))                                             \
    X(EGLBoolean, eglInitialize,                                               \
      (EGLDisplay display, EGLint * major, EGLint * minor))                    \
    X(EGLBoolean, eglMakeCurrent,                                              \
      (EGLDisplay display, EGLSurface draw, EGLSurface read,                   \
       EGLContext context))                                                    \
    X(EGLBoolean, eglSwapBuffers, (EGLDisplay display, EGLSurface surface))    \
    X(EGLSurface, eglCreateWindowSurface,                                      \
      (EGLDisplay display, EGLConfig config,                                   \
       EGLNativeWindowType native_window, EGLint const* attrib_list))          \
    X(EGLBoolean, eglSwapInterval,                                             \
      (EGLDisplay display, EGLint interval))                                   \

using android::base::System;

namespace {
using namespace EglOS;

class EglOsEglDispatcher {
public:
#define DECLARE_EGL_POINTER(return_type, function_name, signature) \
    return_type(EGLAPIENTRY* function_name) signature = nullptr;
    LIST_EGL_FUNCTIONS(DECLARE_EGL_POINTER);

    EglOsEglDispatcher() {
        D("loading %s\n", kEGLLibName);
        char error[256];
        mLib = emugl::SharedLibrary::open(kEGLLibName, error, sizeof(error));
        if (!mLib) {
#ifdef __linux__
            ERR("%s: Could not open EGL library %s [%s]. Trying again with [%s]\n", __FUNCTION__,
                kEGLLibName, error, kEGLLibNameAlt);
            mLib = emugl::SharedLibrary::open(kEGLLibNameAlt, error, sizeof(error));
            if (!mLib) {
                ERR("%s: Could not open EGL library %s [%s]\n", __FUNCTION__,
                    kEGLLibNameAlt, error);
            }
#else
            ERR("%s: Could not open EGL library %s [%s]\n", __FUNCTION__,
                kEGLLibName, error);
#endif
        }

#define LOAD_EGL_POINTER(return_type, function_name, signature)        \
    this->function_name =                                              \
            reinterpret_cast<return_type(GL_APIENTRY*) signature>(     \
                    mLib->findSymbol(#function_name));                 \
    if (!this->function_name) {                                        \
        this->function_name =                                          \
                reinterpret_cast<return_type(GL_APIENTRY*) signature>( \
                        this->eglGetProcAddress(#function_name));      \
    } \
    if (!this->function_name) {                                        \
        D("%s: Could not find %s in underlying EGL library\n",         \
          __FUNCTION__,                                                \
          #function_name);                                             \
    }

        LIST_EGL_FUNCTIONS(LOAD_EGL_POINTER);
    }
    ~EglOsEglDispatcher() = default;

private:
    emugl::SharedLibrary* mLib = nullptr;
};

class EglOsGlLibrary : public GlLibrary {
public:
    EglOsGlLibrary() {
        char error[256];
        mLib = emugl::SharedLibrary::open(kGLES2LibName, error, sizeof(error));
        if (!mLib) {
#ifdef __linux__
            ERR("%s: Could not open GL library %s [%s]. Trying again with [%s]\n", __FUNCTION__,
                kGLES2LibName, error, kGLES2LibNameAlt);
            mLib = emugl::SharedLibrary::open(kGLES2LibNameAlt, error, sizeof(error));
            if (!mLib) {
                ERR("%s: Could not open GL library %s [%s]\n", __FUNCTION__,
                    kGLES2LibNameAlt, error);
            }
#else
            ERR("%s: Could not open GL library %s [%s]\n", __FUNCTION__,
                kGLES2LibName, error);
#endif
        }
    }
    GlFunctionPointer findSymbol(const char* name) {
        if (!mLib) {
            return NULL;
        }
        return reinterpret_cast<GlFunctionPointer>(mLib->findSymbol(name));
    }
    ~EglOsGlLibrary() = default;

private:
    emugl::SharedLibrary* mLib = nullptr;
};

class EglOsEglPixelFormat : public EglOS::PixelFormat {
public:
    EglOsEglPixelFormat(EGLConfig configId, EGLint clientCtxVer)
        : mConfigId(configId), mClientCtxVer(clientCtxVer) {}
    PixelFormat* clone() {
        return new EglOsEglPixelFormat(mConfigId, mClientCtxVer);
    }
    EGLConfig mConfigId;
    EGLint mClientCtxVer;
#ifdef __APPLE__
    int mRedSize = 0;
    int mGreenSize = 0;
    int mBlueSize = 0;
#endif // __APPLE__
};

class EglOsEglContext : public EglOS::Context {
public:
    EglOsEglContext(EglOsEglDispatcher* dispatcher,
                    EGLDisplay display,
                    EGLContext context) :
        mDispatcher(dispatcher),
        mDisplay(display),
        mNativeCtx(context) { }

    ~EglOsEglContext() {
        D("%s %p\n", __FUNCTION__, mNativeCtx);
        if (!mDispatcher->eglDestroyContext(mDisplay, mNativeCtx)) {
            // TODO: print a better error message
        }
    }

    EGLContext context() const {
        return mNativeCtx;
    }

private:
    EglOsEglDispatcher* mDispatcher = nullptr;
    EGLDisplay mDisplay;
    EGLContext mNativeCtx;
};

class EglOsEglSurface : public EglOS::Surface {
public:
    EglOsEglSurface(SurfaceType type,
                    EGLSurface eglSurface,
                    EGLNativeWindowType win = 0)
        : EglOS::Surface(type), mHndl(eglSurface), mWin(win) {}
    EGLSurface getHndl() { return mHndl; }
    EGLNativeWindowType getWin() { return mWin; }

private:
    EGLSurface mHndl;
    EGLNativeWindowType mWin;
};

class EglOsEglDisplay : public EglOS::Display {
public:
    EglOsEglDisplay();
    ~EglOsEglDisplay();
    virtual EglOS::GlesVersion getMaxGlesVersion();
    void queryConfigs(int renderableType,
                      AddConfigCallback* addConfigFunc,
                      void* addConfigOpaque);
    virtual emugl::SmartPtr<Context>
    createContext(EGLint profileMask,
                  const PixelFormat* pixelFormat,
                  Context* sharedContext) override;
    Surface* createPbufferSurface(const PixelFormat* pixelFormat,
                                  const PbufferInfo* info);
    Surface* createWindowSurface(PixelFormat* pf, EGLNativeWindowType win);
    bool releasePbuffer(Surface* pb);
    bool makeCurrent(Surface* read, Surface* draw, Context* context);
    void swapBuffers(Surface* srfc);
    bool isValidNativeWin(Surface* win);
    bool isValidNativeWin(EGLNativeWindowType win);
    bool checkWindowPixelFormatMatch(EGLNativeWindowType win,
                                     const PixelFormat* pixelFormat,
                                     unsigned int* width,
                                     unsigned int* height);

private:
    bool mVerbose = false;
    EGLDisplay mDisplay;
    EglOsEglDispatcher mDispatcher;
    bool mHeadless = false;

#ifdef __linux__
    ::Display* mGlxDisplay = nullptr;
#endif // __linux__
};

EglOsEglDisplay::EglOsEglDisplay() {
    mVerbose = System::getEnvironmentVariable("ANDROID_EMUGL_VERBOSE") == "1";

    mDisplay = mDispatcher.eglGetDisplay(EGL_DEFAULT_DISPLAY);
    mDispatcher.eglInitialize(mDisplay, nullptr, nullptr);
    mDispatcher.eglSwapInterval(mDisplay, 0);
    auto clientExts = mDispatcher.eglQueryString(mDisplay, EGL_EXTENSIONS);

    if (mVerbose) {
        fprintf(stderr, "%s: client exts: [%s]\n", __func__, clientExts);
    }

    mDispatcher.eglBindAPI(EGL_OPENGL_ES_API);
    CHECK_EGL_ERR

    mHeadless = System::getEnvironmentVariable("ANDROID_EMU_HEADLESS") == "1";

#ifdef __linux__
    if (mHeadless) mGlxDisplay = nullptr;
    else mGlxDisplay = XOpenDisplay(0);
#endif // __linux__
};

EglOsEglDisplay::~EglOsEglDisplay() {
#ifdef __linux__
    if (mGlxDisplay) XCloseDisplay(mGlxDisplay);
#endif // __linux__
}

EglOS::GlesVersion EglOsEglDisplay::getMaxGlesVersion() {
    // TODO: Detect and return the highest version like in GLESVersionDetector.cpp
    return EglOS::GlesVersion::ES30;
}

void EglOsEglDisplay::queryConfigs(int renderableType,
                                   AddConfigCallback* addConfigFunc,
                                   void* addConfigOpaque) {
    D("%s\n", __FUNCTION__);
    // ANGLE does not support GLES1 uses core profile engine.
    // Querying underlying EGL with a conservative set of bits.
    renderableType &= ~EGL_OPENGL_ES_BIT;

    const EGLint framebuffer_config_attributes[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE, 1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE, 1,
        EGL_ALPHA_SIZE, 0,
        EGL_NONE,
    };

    EGLint numConfigs = 0;
    mDispatcher.eglChooseConfig(mDisplay, framebuffer_config_attributes, nullptr, 0, &numConfigs);
    CHECK_EGL_ERR
    std::unique_ptr<EGLConfig[]> configs(new EGLConfig[numConfigs]);
    mDispatcher.eglChooseConfig(mDisplay, framebuffer_config_attributes, configs.get(), numConfigs,
                                &numConfigs);
    CHECK_EGL_ERR

    if (mVerbose) {
        fprintf(stderr, "%s: num configs: %d\n", __func__, numConfigs);
    }

    for (int i = 0; i < numConfigs; i++) {
        const EGLConfig cfg = configs.get()[i];
        ConfigInfo configInfo;
        // We do not have recordable_android
        configInfo.recordable_android = 0;
        EGLint _renderableType;
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_RENDERABLE_TYPE,
                                       &_renderableType);
        // We do emulate GLES1
        configInfo.renderable_type = _renderableType | EGL_OPENGL_ES_BIT;

        configInfo.frmt = new EglOsEglPixelFormat(cfg, _renderableType);
        D("config %p renderable type 0x%x\n", cfg, _renderableType);

        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_RED_SIZE,
                                       &configInfo.red_size);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_GREEN_SIZE,
                                       &configInfo.green_size);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_BLUE_SIZE,
                                       &configInfo.blue_size);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_ALPHA_SIZE,
                                       &configInfo.alpha_size);

        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_CONFIG_CAVEAT,
                                       (EGLint*)&configInfo.caveat);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_DEPTH_SIZE,
                                       &configInfo.depth_size);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_LEVEL,
                                       &configInfo.frame_buffer_level);

        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_MAX_PBUFFER_WIDTH,
                                       &configInfo.max_pbuffer_width);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_MAX_PBUFFER_HEIGHT,
                                       &configInfo.max_pbuffer_height);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_MAX_PBUFFER_PIXELS,
                                       &configInfo.max_pbuffer_size);

        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_NATIVE_RENDERABLE,
                                       (EGLint*)&configInfo.native_renderable);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_NATIVE_VISUAL_ID,
                                       &configInfo.native_visual_id);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_NATIVE_VISUAL_TYPE,
                                       &configInfo.native_visual_type);

        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_SAMPLES,
                                       &configInfo.samples_per_pixel);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_STENCIL_SIZE,
                                       &configInfo.stencil_size);

        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_SURFACE_TYPE,
                                       &configInfo.surface_type);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_TRANSPARENT_TYPE,
                                       (EGLint*)&configInfo.transparent_type);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_TRANSPARENT_RED_VALUE,
                                       &configInfo.trans_red_val);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg,
                                       EGL_TRANSPARENT_GREEN_VALUE,
                                       &configInfo.trans_green_val);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg,
                                       EGL_TRANSPARENT_BLUE_VALUE,
                                       &configInfo.trans_blue_val);
        CHECK_EGL_ERR
#ifdef __APPLE__
        ((EglOsEglPixelFormat*)configInfo.frmt)->mRedSize = configInfo.red_size;
        ((EglOsEglPixelFormat*)configInfo.frmt)->mGreenSize = configInfo.green_size;
        ((EglOsEglPixelFormat*)configInfo.frmt)->mBlueSize = configInfo.blue_size;
#endif // __APPLE__
        addConfigFunc(addConfigOpaque, &configInfo);
    }
    D("Host gets %d configs\n", numConfigs);
}

emugl::SmartPtr<Context>
EglOsEglDisplay::createContext(EGLint profileMask,
                               const PixelFormat* pixelFormat,
                               Context* sharedContext) {
    (void)profileMask;

    D("%s\n", __FUNCTION__);
    const EglOsEglPixelFormat* format = (const EglOsEglPixelFormat*)pixelFormat;
    D("with config %p\n", format->mConfigId);
    // Always GLES3
    EGLint attrib_list[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    // TODO: support GLES3.1
    EglOsEglContext* nativeSharedCtx = (EglOsEglContext*)sharedContext;
    EGLContext newNativeCtx = mDispatcher.eglCreateContext(
            mDisplay, format->mConfigId,
            nativeSharedCtx ? nativeSharedCtx->context() : nullptr,
            attrib_list);
    CHECK_EGL_ERR
    emugl::SmartPtr<Context> res =
        std::make_shared<EglOsEglContext>(
            &mDispatcher, mDisplay, newNativeCtx);
    D("%s done\n", __FUNCTION__);
    return res;
}

Surface* EglOsEglDisplay::createPbufferSurface(const PixelFormat* pixelFormat,
                                               const PbufferInfo* info) {
    // D("%s\n", __FUNCTION__);
    // const EglOsEglPixelFormat* format = (const EglOsEglPixelFormat*)pixelFormat;
    // EGLint attrib[] = {EGL_WIDTH,
    //                    info->width,
    //                    EGL_HEIGHT,
    //                    info->height,
    //                    EGL_LARGEST_PBUFFER,
    //                    info->largest,
    //                    EGL_TEXTURE_FORMAT,
    //                    info->format,
    //                    EGL_TEXTURE_TARGET,
    //                    info->target,
    //                    EGL_MIPMAP_TEXTURE,
    //                    info->hasMipmap,
    //                    EGL_NONE};
    // EGLSurface surface = mDispatcher.eglCreatePbufferSurface(
    //         mDisplay, format->mConfigId, attrib);
    // CHECK_EGL_ERR
    // if (surface == EGL_NO_SURFACE) {
    //     D("create pbuffer surface failed\n");
    //     return nullptr;
    // }
    // return new EglOsEglSurface(EglOS::Surface::PBUFFER, surface);
    return new EglOsEglSurface(EglOS::Surface::PBUFFER, 0);
}

Surface* EglOsEglDisplay::createWindowSurface(PixelFormat* pf,
                                              EGLNativeWindowType win) {
    D("%s\n", __FUNCTION__);
    std::vector<EGLint> surface_attribs;
    auto exts = mDispatcher.eglQueryString(mDisplay, EGL_EXTENSIONS);
    if (exts != nullptr && emugl::hasExtension(exts, "EGL_ANGLE_direct_composition")) {
        surface_attribs.push_back(EGL_DIRECT_COMPOSITION_ANGLE);
        surface_attribs.push_back(EGL_TRUE);
    }
    surface_attribs.push_back(EGL_NONE);
    EGLSurface surface = mDispatcher.eglCreateWindowSurface(
            mDisplay, ((EglOsEglPixelFormat*)pf)->mConfigId, win, surface_attribs.data());
    CHECK_EGL_ERR
    if (surface == EGL_NO_SURFACE) {
        D("create window surface failed\n");
        return nullptr;
    }
    return new EglOsEglSurface(EglOS::Surface::WINDOW, surface, win);
}

bool EglOsEglDisplay::releasePbuffer(Surface* pb) {
    D("%s\n", __FUNCTION__);
    if (!pb)
        return false;
    EglOsEglSurface* surface = (EglOsEglSurface*)pb;

    if (!surface->getHndl()) return true;

    bool ret = mDispatcher.eglDestroySurface(mDisplay, surface->getHndl());
    CHECK_EGL_ERR
    D("%s done\n", __FUNCTION__);
    return ret;
}

bool EglOsEglDisplay::makeCurrent(Surface* read,
                                  Surface* draw,
                                  Context* context) {
    D("%s\n", __FUNCTION__);
    EglOsEglSurface* readSfc = (EglOsEglSurface*)read;
    EglOsEglSurface* drawSfc = (EglOsEglSurface*)draw;
    EglOsEglContext* ctx = (EglOsEglContext*)context;
    if (ctx && !readSfc) {
        D("warning: makeCurrent a context without surface\n");
        return false;
    }
    D("%s %p\n", __FUNCTION__, ctx ? ctx->context() : nullptr);
    bool ret = mDispatcher.eglMakeCurrent(
            mDisplay, drawSfc ? drawSfc->getHndl() : EGL_NO_SURFACE,
            readSfc ? readSfc->getHndl() : EGL_NO_SURFACE,
            ctx ? ctx->context() : EGL_NO_CONTEXT);
    if (readSfc) {
        D("make current surface type %d %d\n", readSfc->type(),
          drawSfc->type());
    }
    D("make current %d\n", ret);
    CHECK_EGL_ERR
    return ret;
}

void EglOsEglDisplay::swapBuffers(Surface* surface) {
    D("%s\n", __FUNCTION__);
    EglOsEglSurface* sfc = (EglOsEglSurface*)surface;
    mDispatcher.eglSwapBuffers(mDisplay, sfc->getHndl());
}

bool EglOsEglDisplay::isValidNativeWin(Surface* win) {
    if (!win)
        return false;
    EglOsEglSurface* surface = (EglOsEglSurface*)win;
    return surface->type() == EglOsEglSurface::WINDOW &&
           isValidNativeWin(surface->getWin());
}

bool EglOsEglDisplay::isValidNativeWin(EGLNativeWindowType win) {
#ifdef _WIN32
    return IsWindow(win);
#elif defined(__linux__)
    Window root;
    int t;
    unsigned int u;
    return XGetGeometry(mGlxDisplay, win, &root, &t, &t, &u, &u, &u, &u) != 0;
#else // __APPLE__
    unsigned int width, height;
    return nsGetWinDims(win, &width, &height);
#endif // __APPLE__
}

bool EglOsEglDisplay::checkWindowPixelFormatMatch(EGLNativeWindowType win,
                                 const PixelFormat* pixelFormat,
                                 unsigned int* width,
                                 unsigned int* height) {
#ifdef _WIN32
    RECT r;
    if (!GetClientRect(win, &r)) {
        return false;
    }
    *width = r.right - r.left;
    *height = r.bottom - r.top;
    return true;
#elif defined(__linux__)
    //TODO: to check what does ATI & NVIDIA enforce on win pixelformat
    unsigned int depth, border;
    int x, y;
    Window root;
    return XGetGeometry(
            mGlxDisplay, win, &root, &x, &y, width, height, &border, &depth);
#else // __APPLE__
    bool ret = nsGetWinDims(win, width, height);

    const EglOsEglPixelFormat* format = (EglOsEglPixelFormat*)pixelFormat;
    int r = format->mRedSize;
    int g = format->mGreenSize;
    int b = format->mBlueSize;

    bool match = nsCheckColor(win, r + g + b);

    return ret && match;
#endif // __APPLE__
}

static emugl::LazyInstance<EglOsEglDisplay> sHostDisplay = LAZY_INSTANCE_INIT;

class EglEngine : public EglOS::Engine {
public:
    EglEngine() = default;
    ~EglEngine() = default;

    EglOS::Display* getDefaultDisplay() {
        D("%s\n", __FUNCTION__);
        return sHostDisplay.ptr();
    }
    GlLibrary* getGlLibrary() {
        D("%s\n", __FUNCTION__);
        return &mGlLib;
    }
    virtual EglOS::Surface* createWindowSurface(PixelFormat* pf,
                                                EGLNativeWindowType wnd) {
        D("%s\n", __FUNCTION__);
        return sHostDisplay->createWindowSurface(pf, wnd);
    }

private:
    EglOsGlLibrary mGlLib;
};

}  // namespace

static emugl::LazyInstance<EglEngine> sHostEngine = LAZY_INSTANCE_INIT;

namespace EglOS {
Engine* getEgl2EglHostInstance() {
    D("%s\n", __FUNCTION__);
    return sHostEngine.ptr();
}
}  // namespace EglOS
