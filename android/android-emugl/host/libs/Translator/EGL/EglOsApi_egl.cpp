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

#include "EglOsApi.h"

#include "emugl/common/lazy_instance.h"
#include "emugl/common/shared_library.h"
#include "GLcommon/GLLibrary.h"
#include "OpenglCodecCommon/ErrorLog.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <memory>

#define DEBUG 1
#if DEBUG
#define D(...)  fprintf(stderr, __VA_ARGS__)
#define CHECK_EGL_ERR {EGLint err = mDispatcher.eglGetError(); if (err != EGL_SUCCESS) D("%s: %s %d get egl error %d\n", __FUNCTION__, __FILE__, __LINE__, err);}
#else
#define D(...)  ((void)0)
#define CHECK_EGL_ERR ((void)0)
#endif

// List of WGL functions of interest to probe with GetProcAddress()
#define LIST_WGL_FUNCTIONS(X) \
    X(EGLBoolean, eglChooseConfig, (EGLDisplay display, EGLint const * attrib_list, EGLConfig * configs, EGLint config_size, EGLint * num_config)) \
    X(EGLContext, eglCreateContext, (EGLDisplay display, EGLConfig config, EGLContext share_context, EGLint const * attrib_list)) \
    X(EGLSurface, eglCreatePbufferSurface, (EGLDisplay display, EGLConfig config, EGLint const * attrib_list)) \
    X(EGLBoolean, eglDestroyContext, (EGLDisplay display, EGLContext context)) \
    X(EGLBoolean, eglDestroySurface, (EGLDisplay display, EGLSurface surface)) \
    X(EGLBoolean, eglGetConfigAttrib, (EGLDisplay display, EGLConfig config, EGLint attribute, EGLint * value)) \
    X(EGLDisplay, eglGetDisplay, (NativeDisplayType native_display)) \
    X(EGLint, eglGetError, (void)) \
    X(EGLBoolean, eglInitialize, (EGLDisplay display, EGLint * major, EGLint * minor)) \
    X(EGLBoolean, eglMakeCurrent, (EGLDisplay display, EGLSurface draw, EGLSurface read, EGLContext context)) \
    X(EGLBoolean, eglSwapBuffers, (EGLDisplay display, EGLSurface surface)) \
    
  

//X(void*, eglGetProcAddress, (const char* functionName))

namespace {
    
using namespace EglOS;

class EglOsEglDispatcher {
public:
#define DECLARE_EGL_POINTER(return_type, function_name, signature) \
    return_type (EGLAPIENTRY* function_name) signature = nullptr;
    LIST_WGL_FUNCTIONS(DECLARE_EGL_POINTER);

    EglOsEglDispatcher() {
        static const char kLibName[] = "ANGLE_libEGL.dll";
        D("loading %s\n", kLibName);
        char error[256];
        mLib = emugl::SharedLibrary::open(kLibName, error, sizeof(error));
        if (!mLib) {
            ERR("%s: Could not open GL library %s [%s]\n",
                __FUNCTION__, kLibName, error);
        }

#define LOAD_WGL_POINTER(return_type, function_name, signature) \
        this->function_name = reinterpret_cast< \
                return_type (GL_APIENTRY*) signature>( \
                        mLib->findSymbol(#function_name)); \
        if (!this->function_name) { \
            ERR("%s: Could not find %s in GL library\n", __FUNCTION__, \
                    #function_name); \
        }

        LIST_WGL_FUNCTIONS(LOAD_WGL_POINTER);
    }
    ~EglOsEglDispatcher() = default;
private:
    emugl::SharedLibrary* mLib = nullptr;
};

class EglOsGlLibrary : public GlLibrary {
public:
    EglOsGlLibrary() {
        const char* kLibName = "ANGLE_libGLESv2.dll";
        char error[256];
        mLib = emugl::SharedLibrary::open(kLibName, error, sizeof(error));
        if (!mLib) {
            ERR("%s: Could not open GL library %s [%s]\n",
                __FUNCTION__, kLibName, error);
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
        : mConfigId(configId) ,
          mClientCtxVer(clientCtxVer) {}
    PixelFormat* clone() {
        return new EglOsEglPixelFormat(mConfigId, mClientCtxVer);
    }
    EGLConfig mConfigId;
    EGLint mClientCtxVer;
};

struct EglOsEglContext : public EglOS::Context {
    EGLContext mNativeCtx;
};

class EglOsEglSurface : public EglOS::Surface {
public:
    EglOsEglSurface(SurfaceType type, EGLSurface eglSurface)
        : EglOS::Surface(type),
          mHndl(eglSurface) {}
    EGLSurface getHndl() {return mHndl;}
private:
    EGLSurface mHndl;
};

class EglOsEglDisplay : public EglOS::Display {
public:
    EglOsEglDisplay() {
        mDisplay = mDispatcher.eglGetDisplay(EGL_DEFAULT_DISPLAY);
        mDispatcher.eglInitialize(mDisplay, nullptr, nullptr);
        CHECK_EGL_ERR
    };
    ~EglOsEglDisplay() = default;
    void queryConfigs(int renderableType,
                      AddConfigCallback* addConfigFunc,
                      void* addConfigOpaque);
    Context* createContext(
            const PixelFormat* pixelFormat, Context* sharedContext);
    bool destroyContext(Context* context);
    Surface* createPbufferSurface(
            const PixelFormat* pixelFormat, const PbufferInfo* info);
    bool releasePbuffer(Surface* pb);
    bool makeCurrent(Surface* read,
                             Surface* draw,
                             Context* context);
    void swapBuffers(Surface* srfc);
    bool isValidNativeWin(Surface* win) {return true;} // TODO
    bool isValidNativeWin(EGLNativeWindowType win) {return true;} // TODO
    bool checkWindowPixelFormatMatch(EGLNativeWindowType win,
                                             const PixelFormat* pixelFormat,
                                             unsigned int* width,
                                             unsigned int* height) {
        return true; // TODO
    }
private:
    EGLDisplay mDisplay;
    EglOsEglDispatcher mDispatcher;
};

void EglOsEglDisplay::queryConfigs(int renderableType,
        AddConfigCallback* addConfigFunc, void* addConfigOpaque) {
    D("%s\n", __FUNCTION__);
    /*const EGLint attribList[] = {
        EGL_ALPHA_SIZE, EGL_DONT_CARE,
        EGL_RED_SIZE, EGL_DONT_CARE,
        EGL_BLUE_SIZE, EGL_DONT_CARE,
        EGL_GREEN_SIZE, EGL_DONT_CARE,
        EGL_DEPTH_SIZE, EGL_DONT_CARE,
        EGL_STENCIL_SIZE, EGL_DONT_CARE,

        EGL_BUFFER_SIZE, EGL_DONT_CARE,

        //EGL_CONFORMANT, EGL_DONT_CARE,
        //EGL_RENDERABLE_TYPE, EGL_DONT_CARE,

        //EGL_SURFACE_TYPE, EGL_PBUFFER_BIT | EGL_WINDOW_BIT,
        //EGL_SURFACE_TYPE, EGL_DONT_CARE,

        EGL_NONE
    };*/
    const EGLint attribList[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_NONE};
    EGLint numConfigs = 0;
    mDispatcher.eglChooseConfig(mDisplay, attribList, nullptr, 0, &numConfigs);
    CHECK_EGL_ERR
    std::unique_ptr<EGLConfig[]> configs(new EGLConfig[numConfigs]);
    mDispatcher.eglChooseConfig(mDisplay, attribList, configs.get(), numConfigs,
        &numConfigs);
    CHECK_EGL_ERR
    for (int i = 0; i < numConfigs; i++) {
        const EGLConfig cfg = configs.get()[i];
        ConfigInfo configInfo;
        configInfo.config_id = i;
        // We do not have recordable_android
        configInfo.recordable_android = 0;
        EGLint _renderableType;
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_RENDERABLE_TYPE, &_renderableType);
        configInfo.renderable_type = _renderableType;
        
        configInfo.frmt = new EglOsEglPixelFormat(cfg, _renderableType);
        D("config %p renderable type 0x%x\n", cfg, _renderableType);

        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_RED_SIZE, &configInfo.red_size);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_GREEN_SIZE, &configInfo.green_size);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_BLUE_SIZE, &configInfo.blue_size);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_ALPHA_SIZE, &configInfo.alpha_size);

        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_CONFIG_CAVEAT, (EGLint*)&configInfo.caveat);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_DEPTH_SIZE, &configInfo.depth_size);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_LEVEL, &configInfo.frame_buffer_level);

        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_MAX_PBUFFER_WIDTH, &configInfo.max_pbuffer_width);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_MAX_PBUFFER_HEIGHT, &configInfo.max_pbuffer_height);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_MAX_PBUFFER_PIXELS, &configInfo.max_pbuffer_size);

        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_NATIVE_RENDERABLE, (EGLint*)&configInfo.native_renderable);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_RENDERABLE_TYPE, &configInfo.renderable_type);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_NATIVE_VISUAL_ID, &configInfo.native_visual_id);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_NATIVE_VISUAL_TYPE, &configInfo.native_visual_type);

        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_SAMPLES, &configInfo.samples_per_pixel);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_STENCIL_SIZE, &configInfo.stencil_size);

        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_SURFACE_TYPE, &configInfo.surface_type);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_TRANSPARENT_TYPE, (EGLint*)&configInfo.transparent_type);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_TRANSPARENT_RED_VALUE, &configInfo.trans_red_val);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_TRANSPARENT_GREEN_VALUE, &configInfo.trans_green_val);
        mDispatcher.eglGetConfigAttrib(mDisplay, cfg, EGL_TRANSPARENT_BLUE_VALUE, &configInfo.trans_blue_val);
        CHECK_EGL_ERR
        addConfigFunc(addConfigOpaque, &configInfo);
    }
    D("Host gets %d configs\n", numConfigs);
}

Context* EglOsEglDisplay::createContext(const PixelFormat* pixelFormat,
        Context* sharedContext) {
    D("%s\n", __FUNCTION__);
    const EglOsEglPixelFormat* format = (const EglOsEglPixelFormat*)pixelFormat;
    // Always GLES2
    EGLint attrib_list[] = {EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE};
    EglOsEglContext* nativeSharedCtx = (EglOsEglContext*)sharedContext;
    EglOsEglContext* ctx = new EglOsEglContext();
    D("with config %p\n", format->mConfigId);
    ctx->mNativeCtx = mDispatcher.eglCreateContext(mDisplay,
        format->mConfigId,
        nativeSharedCtx ? nativeSharedCtx->mNativeCtx : nullptr, attrib_list);
    CHECK_EGL_ERR
    D("%s done\n", __FUNCTION__);
    return ctx;
}

bool EglOsEglDisplay::destroyContext(Context* context) {
    D("%s\n", __FUNCTION__);
    if (!context) return false;
    EglOsEglContext* ctx = (EglOsEglContext*)context;
    if (!mDispatcher.eglDestroyContext(mDisplay, ctx->mNativeCtx)) {
        // TODO: print a better error message
        return false;
    }
    delete ctx;
    return true;
}

Surface* EglOsEglDisplay::createPbufferSurface(const PixelFormat* pixelFormat,
        const PbufferInfo* info) {
    D("%s\n", __FUNCTION__);
    const EglOsEglPixelFormat* format = (const EglOsEglPixelFormat*)pixelFormat;
    EGLint attrib[] = {
        EGL_WIDTH, info->width,
        EGL_HEIGHT, info->height,
        EGL_LARGEST_PBUFFER, info->largest,
        EGL_TEXTURE_FORMAT, info->format,
        EGL_TEXTURE_TARGET, info->target,
        EGL_MIPMAP_TEXTURE, info->hasMipmap,
        EGL_NONE
    };
    EGLSurface surface = mDispatcher.eglCreatePbufferSurface(
            mDisplay, format->mConfigId, attrib);
    CHECK_EGL_ERR
    if (surface == EGL_NO_SURFACE) {
        // TODO: print the error message
        return nullptr;
    }
    return new EglOsEglSurface(EglOS::Surface::PBUFFER, surface);
}

bool EglOsEglDisplay::releasePbuffer(Surface* pb) {
    D("%s\n", __FUNCTION__);
    if (!pb) return false;
    EglOsEglSurface* surface = (EglOsEglSurface*)pb;
    bool ret = mDispatcher.eglDestroySurface(mDisplay, surface->getHndl());
    CHECK_EGL_ERR
    D("%s done\n", __FUNCTION__);
    return ret;
}

bool EglOsEglDisplay::makeCurrent(Surface* read, Surface* draw,
        Context* context) {
    D("%s\n", __FUNCTION__);
    EglOsEglSurface* readSfc = (EglOsEglSurface*)read;
    EglOsEglSurface* drawSfc = (EglOsEglSurface*)draw;
    EglOsEglContext* ctx = (EglOsEglContext*)context;
    return mDispatcher.eglMakeCurrent(mDisplay,
        drawSfc ? drawSfc->getHndl() : nullptr,
        readSfc ? readSfc->getHndl() : nullptr,
        ctx ? ctx->mNativeCtx : nullptr);
}

void EglOsEglDisplay::swapBuffers(Surface* surface) {
    D("%s\n", __FUNCTION__);
    EglOsEglSurface* sfc = (EglOsEglSurface*)surface;
    mDispatcher.eglSwapBuffers(mDisplay, sfc->getHndl());
}

class EglEngine : public EglOS::Engine {
public:
    EglEngine() = default;
    ~EglEngine() = default;

    Display* getDefaultDisplay() {
        D("%s\n", __FUNCTION__);
        return new EglOsEglDisplay();
    }
    GlLibrary* getGlLibrary() {
        D("%s\n", __FUNCTION__);
        return &mGlLib;
    }
    virtual EglOS::Surface* createWindowSurface(EGLNativeWindowType wnd) {
        D("%s\n", __FUNCTION__);
        return new EglOsEglSurface(EglOsEglSurface::WINDOW, wnd);
    }
private:
    EglOsGlLibrary mGlLib;
};

} // namespace

static emugl::LazyInstance<EglEngine> sHostEngine = LAZY_INSTANCE_INIT;

// static
EglOS::Engine* EglOS::Engine::getHostInstance() {
    D("%s\n", __FUNCTION__);
    return sHostEngine.ptr();
}


