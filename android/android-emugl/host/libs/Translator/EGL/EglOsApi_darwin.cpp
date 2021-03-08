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

#include "MacNative.h"

#include "android/base/containers/Lookup.h"

#include "emugl/common/lazy_instance.h"
#include "emugl/common/shared_library.h"
#include "GLcommon/GLLibrary.h"
#include "OpenglCodecCommon/ErrorLog.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <numeric>
#include <unordered_map>

#define MAX_PBUFFER_MIPMAP_LEVEL 1

using FinalizedConfigKey = std::pair<bool, int>;
struct FinalizedConfigHash {
    std::size_t operator () (const FinalizedConfigKey& p) const {
        return p.second + 9001 * (p.first ? 1 : 0);
    }
};
using FinalizedConfigMap =
    std::unordered_map<FinalizedConfigKey, void*, FinalizedConfigHash>;

static emugl::LazyInstance<FinalizedConfigMap> sFinalizedConfigs =
    LAZY_INSTANCE_INIT;

namespace {

class MacSurface : public EglOS::Surface {
public:
    MacSurface(void* handle, SurfaceType type) :
            Surface(type), m_handle(handle) {}

    void* handle() const { return m_handle; }

    bool hasMipmap() const { return m_hasMipmap; }
    void setHasMipmap(bool value) { m_hasMipmap = value; }

    static MacSurface* from(EglOS::Surface* s) {
        return static_cast<MacSurface*>(s);
    }

private:
    void* m_handle = nullptr;
    bool m_hasMipmap = false;
};

class MacContext : public EglOS::Context {
public:
    explicit MacContext(bool isCoreProfile, void* context) :
        EglOS::Context(isCoreProfile), mContext(context) {}

    ~MacContext() {
        nsDestroyContext(mContext);
    }

    void* context() const { return mContext; }

    static void* from(EglOS::Context* c) {
        return static_cast<MacContext*>(c)->context();
    }

private:
    void* mContext = nullptr;
};

class MacNativeSupportInfo {
public:
    MacNativeSupportInfo() :
        numNativeFormats(getNumPixelFormats()),
        maxOpenGLProfile((MacOpenGLProfileVersions)setupCoreProfileNativeFormats()) { }
    int numNativeFormats = 0;
    MacOpenGLProfileVersions maxOpenGLProfile =
        MAC_OPENGL_PROFILE_LEGACY;
};

static emugl::LazyInstance<MacNativeSupportInfo> sSupportInfo =
    LAZY_INSTANCE_INIT;

class MacPixelFormat : public EglOS::PixelFormat {
public:
    MacPixelFormat(int handle, int redSize, int greenSize, int blueSize) :
            mHandle(handle),
            mRedSize(redSize),
            mGreenSize(greenSize),
            mBlueSize(blueSize) {}

    EglOS::PixelFormat* clone() {
        return new MacPixelFormat(mHandle, mRedSize, mGreenSize, mBlueSize);
    }

    int handle() const { return mHandle; }
    int redSize() const { return mRedSize; }
    int greenSize() const { return mGreenSize; }
    int blueSize() const { return mBlueSize; }

    static const MacPixelFormat* from(const EglOS::PixelFormat* f) {
        return static_cast<const MacPixelFormat*>(f);
    }

private:
    MacPixelFormat();
    MacPixelFormat(const MacPixelFormat& other);

    int mHandle = 0;
    int mRedSize = 0;
    int mGreenSize = 0;
    int mBlueSize = 0;
};


void pixelFormatToConfig(int index,
                         EglOS::AddConfigCallback addConfigFunc,
                         void* addConfigOpaque) {
    EglOS::ConfigInfo info = {};

    info.surface_type = EGL_WINDOW_BIT | EGL_PBUFFER_BIT;

    //default values
    info.native_visual_id = 0;
    info.native_visual_type = EGL_NONE;
    info.caveat = EGL_NONE;
    info.native_renderable = EGL_FALSE;

    info.renderable_type = EGL_OPENGL_ES_BIT |
                           EGL_OPENGL_ES2_BIT |
                           EGL_OPENGL_ES3_BIT;

    info.max_pbuffer_width = PBUFFER_MAX_WIDTH;
    info.max_pbuffer_height = PBUFFER_MAX_HEIGHT;
    info.max_pbuffer_size = PBUFFER_MAX_PIXELS;
    info.samples_per_pixel = 0;
    info.frame_buffer_level = 0;
    info.trans_red_val = 0;
    info.trans_green_val = 0;
    info.trans_blue_val = 0;

    info.transparent_type = EGL_NONE;

    /* All configs can end up having an alpha channel even if none was requested.
     * The default config chooser in GLSurfaceView will therefore not find any
     * matching config. Thus, make sure alpha is zero (or at least signalled as
     * zero to the calling EGL layer) for the configs where it was intended to
     * be zero. */
    info.alpha_size = getPixelFormatAttrib(index, MAC_ALPHA_SIZE);

    info.depth_size = getPixelFormatAttrib(index, MAC_DEPTH_SIZE);
    info.stencil_size = getPixelFormatAttrib(index, MAC_STENCIL_SIZE);
    info.samples_per_pixel = getPixelFormatAttrib(index, MAC_SAMPLES_PER_PIXEL);

    //TODO: ask guy if it is OK
    GLint colorSize = 0;
    colorSize = (GLint)getPixelFormatAttrib(index, MAC_COLOR_SIZE);
    info.red_size = info.green_size = info.blue_size = (colorSize / 4);

    info.frmt = new MacPixelFormat(
            index, info.red_size, info.green_size, info.blue_size);

    (*addConfigFunc)(addConfigOpaque, &info);
}


class MacDisplay : public EglOS::Display {
public:
    explicit MacDisplay(EGLNativeDisplayType dpy) : mDpy(dpy) {}

    virtual EglOS::GlesVersion getMaxGlesVersion() {
        switch (sSupportInfo->maxOpenGLProfile) {
        case MAC_OPENGL_PROFILE_LEGACY:
            return EglOS::GlesVersion::ES2;
        case MAC_OPENGL_PROFILE_3_2:
        case MAC_OPENGL_PROFILE_4_1:
            return EglOS::GlesVersion::ES30;
        default:
            return EglOS::GlesVersion::ES2;
        }
    }

    virtual void queryConfigs(int renderableType,
                              EglOS::AddConfigCallback* addConfigFunc,
                              void* addConfigOpaque) {
        for (int i = 0; i < sSupportInfo->numNativeFormats; i++) {
            pixelFormatToConfig(
                i,
                addConfigFunc,
                addConfigOpaque);
        }

        // Also disable vsync.
        int bestSwapInterval = 0;
        nsSwapInterval(&bestSwapInterval);
    }

    virtual bool isValidNativeWin(EglOS::Surface* win) {

        if (!win) { return false; }

        if (win->type() != MacSurface::WINDOW) {
            return false;
        } else {
            return isValidNativeWin(MacSurface::from(win)->handle());
        }
    }

    virtual bool isValidNativeWin(EGLNativeWindowType win) {
        unsigned int width, height;
        if (!win) return false;
        return nsGetWinDims(win, &width, &height);
    }

    virtual bool checkWindowPixelFormatMatch(
            EGLNativeWindowType win,
            const EglOS::PixelFormat* pixelFormat,
            unsigned int* width,
            unsigned int* height) {
        bool ret = nsGetWinDims(win, width, height);

        const MacPixelFormat* format = MacPixelFormat::from(pixelFormat);
        int r = format->redSize();
        int g = format->greenSize();
        int b = format->blueSize();

        bool match = nsCheckColor(win, r + g + b);

        return ret && match;
    }

    virtual emugl::SmartPtr<EglOS::Context> createContext(
            EGLint profileMask,
            const EglOS::PixelFormat* pixelFormat,
            EglOS::Context* sharedContext) {

        void* macSharedContext =
                sharedContext ? MacContext::from(sharedContext) : NULL;

        bool isCoreProfile =
            profileMask & EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR;

        auto key = std::make_pair(isCoreProfile,
                                  MacPixelFormat::from(pixelFormat)->handle());

        void* nsFormat = nullptr;
        if (auto format = android::base::find(sFinalizedConfigs.get(), key)) {
            nsFormat = *format;
        } else {
            nsFormat =
                finalizePixelFormat(isCoreProfile,
                        MacPixelFormat::from(pixelFormat)->handle());
            sFinalizedConfigs.get()[key] = nsFormat;
        }

        return std::make_shared<MacContext>(
                   isCoreProfile,
                   nsCreateContext(nsFormat,
                                   macSharedContext));
    }

    virtual EglOS::Surface* createPbufferSurface(
            const EglOS::PixelFormat* pixelFormat,
            const EglOS::PbufferInfo* info) {
        GLenum glTexFormat = GL_RGBA, glTexTarget = GL_TEXTURE_2D;
        switch (info->format) {
        case EGL_TEXTURE_RGB:
            glTexFormat = GL_RGB;
            break;
        case EGL_TEXTURE_RGBA:
            glTexFormat = GL_RGBA;
            break;
        }
        EGLint maxMipmap = info->hasMipmap ? MAX_PBUFFER_MIPMAP_LEVEL : 0;
        bool isLegacyOpenGL =
            sSupportInfo->maxOpenGLProfile == MAC_OPENGL_PROFILE_LEGACY;
        MacSurface* result = new MacSurface(
            isLegacyOpenGL
                ? nsCreatePBuffer(glTexTarget, glTexFormat, maxMipmap,
                                  info->width, info->height)
                : nullptr,
            MacSurface::PBUFFER);

        result->setHasMipmap(info->hasMipmap);
        return result;
    }

    virtual bool releasePbuffer(EglOS::Surface* pb) {
        if (pb) {
            nsDestroyPBuffer(MacSurface::from(pb)->handle());
            delete pb;
        }
        return true;
    }

    virtual bool makeCurrent(EglOS::Surface* read,
                             EglOS::Surface* draw,
                             EglOS::Context* ctx) {
        // check for unbind
        if (ctx == NULL && read == NULL && draw == NULL) {
            nsWindowMakeCurrent(NULL, NULL);
            return true;
        }
        else if (ctx == NULL || read == NULL || draw == NULL) {
            // error !
            return false;
        }

        //dont supporting diffrent read & draw surfaces on Mac
        if (read != draw) {
            return false;
        }
        switch (draw->type()) {
        case MacSurface::WINDOW:
            nsWindowMakeCurrent(MacContext::from(ctx),
                                MacSurface::from(draw)->handle());
            break;
        case MacSurface::PBUFFER:
        {
            MacSurface* macdraw = MacSurface::from(draw);
            int mipmapLevel = macdraw->hasMipmap() ? MAX_PBUFFER_MIPMAP_LEVEL : 0;
            nsPBufferMakeCurrent(MacContext::from(ctx),
                                 macdraw->handle(), mipmapLevel);
            break;
        }
        default:
            return false;
        }
        return true;
    }

    virtual void swapBuffers(EglOS::Surface* srfc) {
        nsSwapBuffers();
    }

    EGLNativeDisplayType dpy() const { return mDpy; }

private:
    EglOS::GlesVersion mMaxGlesVersion =
        EglOS::GlesVersion::ES2;
    EGLNativeDisplayType mDpy = {};
};

class MacGlLibrary : public GlLibrary {
public:
    MacGlLibrary() {
        static const char kLibName[] =
                "/System/Library/Frameworks/OpenGL.framework/OpenGL";
        char error[256];
        mLib = emugl::SharedLibrary::open(kLibName, error, sizeof(error));
        if (!mLib) {
            ERR("%s: Could not open GL library %s [%s]\n",
                __FUNCTION__, kLibName, error);
        }
    }

    ~MacGlLibrary() {
    }

    // override
    virtual GlFunctionPointer findSymbol(const char* name) {
        if (!mLib) {
            return NULL;
        }
        return reinterpret_cast<GlFunctionPointer>(mLib->findSymbol(name));
    }

private:
    emugl::SharedLibrary* mLib = nullptr;
};

class MacEngine : public EglOS::Engine {
public:
    virtual EglOS::Display* getDefaultDisplay() {
        return new MacDisplay(0);
    }

    virtual GlLibrary* getGlLibrary() {
        return &mGlLib;
    }

    virtual EglOS::Surface* createWindowSurface(EglOS::PixelFormat* cfg,
                                                EGLNativeWindowType wnd) {
        return new MacSurface(wnd, MacSurface::WINDOW);
    }

private:
    MacGlLibrary mGlLib;
};

emugl::LazyInstance<MacEngine> sHostEngine = LAZY_INSTANCE_INIT;

}  // namespace


// static
EglOS::Engine* EglOS::Engine::getHostInstance() {
    return sHostEngine.ptr();
}

