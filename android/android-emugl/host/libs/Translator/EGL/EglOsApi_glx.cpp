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

#include "android/base/synchronization/Lock.h"

#include "CoreProfileConfigs.h"
#include "emugl/common/lazy_instance.h"
#include "emugl/common/mutex.h"
#include "emugl/common/shared_library.h"
#include "GLcommon/GLLibrary.h"

#include "OpenglCodecCommon/ErrorLog.h"

#include <string.h>
#include <X11/Xlib.h>
#include <GL/glx.h>

#include <EGL/eglext.h>

#include <unordered_map>

#define DEBUG_PBUF_POOL 0

// TODO: Replace with latency tracker.
#define PROFILE_SLOW(tag)

namespace {

typedef Display X11Display;

class ErrorHandler{
public:
    ErrorHandler(EGLNativeDisplayType dpy);
    ~ErrorHandler();
    int getLastError() const { return s_lastErrorCode; }

private:
    static int s_lastErrorCode;
    int (*m_oldErrorHandler)(Display *, XErrorEvent *) = nullptr;
    static emugl::Mutex s_lock;
    static int errorHandlerProc(EGLNativeDisplayType dpy,XErrorEvent* event);
};

// static
int ErrorHandler::s_lastErrorCode = 0;

// static
emugl::Mutex ErrorHandler::s_lock;

ErrorHandler::ErrorHandler(EGLNativeDisplayType dpy) {
   emugl::Mutex::AutoLock mutex(s_lock);
   XSync(dpy,False);
   s_lastErrorCode = 0;
   m_oldErrorHandler = XSetErrorHandler(errorHandlerProc);
}

ErrorHandler::~ErrorHandler() {
   emugl::Mutex::AutoLock mutex(s_lock);
   XSetErrorHandler(m_oldErrorHandler);
   s_lastErrorCode = 0;
}

int ErrorHandler::errorHandlerProc(EGLNativeDisplayType dpy,
                                   XErrorEvent* event) {
    s_lastErrorCode = event->error_code;
    return 0;
}

#define IS_SUCCESS(a) \
        do { if (a != Success) return 0; } while (0)

#define EXIT_IF_FALSE(a) \
        do { if (a != Success) return; } while (0)

class GlxLibrary : public GlLibrary {
public:
    typedef GlFunctionPointer (ResolverFunc)(const char* name);

    // Important: Use libGL.so.1 explicitly, because it will always link to
    // the vendor-specific version of the library. libGL.so might in some
    // cases, depending on bad ldconfig configurations, link to the wrapper
    // lib that doesn't behave the same.
    GlxLibrary() {
        static const char kLibName[] = "libGL.so.1";
        char error[256];
        mLib = emugl::SharedLibrary::open(kLibName, error, sizeof(error));
        if (!mLib) {
            ERR("%s: Could not open GL library %s [%s]\n",
                __func__, kLibName, error);
            return;
        }
        // NOTE: Don't use glXGetProcAddress here, only glXGetProcAddressARB
        // is guaranteed to be supported by vendor-specific libraries.
        static const char kResolverName[] = "glXGetProcAddressARB";
        mResolver = reinterpret_cast<ResolverFunc*>(
                mLib->findSymbol(kResolverName));
        if (!mResolver) {
            ERR("%s: Could not find resolver %s in %s\n",
                __func__, kResolverName, kLibName);
            mLib = NULL;
        }
    }

    ~GlxLibrary() {
    }

    // override
    virtual GlFunctionPointer findSymbol(const char* name) {
        if (!mLib) {
            return NULL;
        }
        GlFunctionPointer ret = (*mResolver)(name);
        if (!ret) {
            ret = reinterpret_cast<GlFunctionPointer>(mLib->findSymbol(name));
        }
        return ret;
    }

private:
    emugl::SharedLibrary* mLib = nullptr;
    ResolverFunc* mResolver = nullptr;
};

emugl::LazyInstance<GlxLibrary> sGlxLibrary = LAZY_INSTANCE_INIT;

// Implementation of EglOS::PixelFormat based on GLX.
class GlxPixelFormat : public EglOS::PixelFormat {
public:
    explicit GlxPixelFormat(GLXFBConfig fbconfig) : mFbConfig(fbconfig) {}

    virtual EglOS::PixelFormat* clone() {
        return new GlxPixelFormat(mFbConfig);
    }

    GLXFBConfig fbConfig() const { return mFbConfig; }

    static GLXFBConfig from(const EglOS::PixelFormat* f) {
        return static_cast<const GlxPixelFormat*>(f)->fbConfig();
    }

private:
    GLXFBConfig mFbConfig = nullptr;
};

// Implementation of EglOS::Surface based on GLX.
class GlxSurface : public EglOS::Surface {
public:
    GlxSurface(GLXDrawable drawable, GLXFBConfig fbConfig, SurfaceType type) :
            Surface(type), mFbConfig(fbConfig), mDrawable(drawable) {}

    GLXDrawable drawable() const { return mDrawable; }
    GLXFBConfig config() const { return mFbConfig; }

    // Helper routine to down-cast an EglOS::Surface and extract
    // its drawable.
    static GLXDrawable drawableFor(EglOS::Surface* surface) {
        return static_cast<GlxSurface*>(surface)->drawable();
    }

    // Helper routine to down-cast an EglOS::Surface and extract
    // its config.
    static GLXFBConfig configFor(EglOS::Surface* surface) {
        return static_cast<GlxSurface*>(surface)->config();
    }

private:
    GLXFBConfig mFbConfig = 0;
    GLXDrawable mDrawable = 0;
};

void pixelFormatToConfig(EGLNativeDisplayType dpy,
                         int renderableType,
                         GLXFBConfig frmt,
                         EglOS::AddConfigCallback* addConfigFunc,
                         void* addConfigOpaque) {
    EglOS::ConfigInfo info;
    int  tmp;

    memset(&info, 0, sizeof(info));

    EXIT_IF_FALSE(glXGetFBConfigAttrib(dpy, frmt, GLX_TRANSPARENT_TYPE, &tmp));
    if (tmp == GLX_TRANSPARENT_INDEX) {
        return; // not supporting transparent index
    } else if (tmp == GLX_NONE) {
        info.transparent_type = EGL_NONE;
        info.trans_red_val = 0;
        info.trans_green_val = 0;
        info.trans_blue_val = 0;
    } else {
        info.transparent_type = EGL_TRANSPARENT_RGB;

        EXIT_IF_FALSE(glXGetFBConfigAttrib(
                dpy, frmt, GLX_TRANSPARENT_RED_VALUE, &info.trans_red_val));
        EXIT_IF_FALSE(glXGetFBConfigAttrib(
                dpy, frmt, GLX_TRANSPARENT_GREEN_VALUE, &info.trans_green_val));
        EXIT_IF_FALSE(glXGetFBConfigAttrib(
                dpy, frmt, GLX_TRANSPARENT_BLUE_VALUE, &info.trans_blue_val));
    }

    //
    // filter out single buffer configurations
    //
    int doubleBuffer = 0;
    EXIT_IF_FALSE(glXGetFBConfigAttrib(
            dpy, frmt, GLX_DOUBLEBUFFER, &doubleBuffer));
    if (!doubleBuffer) {
        return;
    }

    EXIT_IF_FALSE(glXGetFBConfigAttrib(
            dpy ,frmt, GLX_RED_SIZE, &info.red_size));
    EXIT_IF_FALSE(glXGetFBConfigAttrib(
            dpy ,frmt, GLX_GREEN_SIZE, &info.green_size));
    EXIT_IF_FALSE(glXGetFBConfigAttrib(
            dpy ,frmt, GLX_BLUE_SIZE, &info.blue_size));
    EXIT_IF_FALSE(glXGetFBConfigAttrib(
            dpy ,frmt, GLX_ALPHA_SIZE, &info.alpha_size));
    EXIT_IF_FALSE(glXGetFBConfigAttrib(
            dpy ,frmt, GLX_DEPTH_SIZE, &info.depth_size));
    EXIT_IF_FALSE(glXGetFBConfigAttrib(
            dpy ,frmt, GLX_STENCIL_SIZE, &info.stencil_size));

    info.renderable_type = renderableType;
    int nativeRenderable = 0;
    EXIT_IF_FALSE(glXGetFBConfigAttrib(
            dpy, frmt, GLX_X_RENDERABLE, &nativeRenderable));
    info.native_renderable = !!nativeRenderable;

    EXIT_IF_FALSE(glXGetFBConfigAttrib(
            dpy, frmt, GLX_X_VISUAL_TYPE, &info.native_visual_type));

    EXIT_IF_FALSE(glXGetFBConfigAttrib(
            dpy, frmt, GLX_VISUAL_ID, &info.native_visual_id));

    //supported surfaces types
    info.surface_type = 0;
    EXIT_IF_FALSE(glXGetFBConfigAttrib(dpy, frmt, GLX_DRAWABLE_TYPE, &tmp));
    if (tmp & GLX_WINDOW_BIT && info.native_visual_id != 0) {
        info.surface_type |= EGL_WINDOW_BIT;
    } else {
        info.native_visual_id = 0;
        info.native_visual_type = EGL_NONE;
    }
    if (tmp & GLX_PBUFFER_BIT) {
        info.surface_type |= EGL_PBUFFER_BIT;
    }

    info.caveat = 0;
    EXIT_IF_FALSE(glXGetFBConfigAttrib(dpy, frmt, GLX_CONFIG_CAVEAT, &tmp));
    if (tmp == GLX_NONE) {
        info.caveat = EGL_NONE;
    } else if (tmp == GLX_SLOW_CONFIG) {
        info.caveat = EGL_SLOW_CONFIG;
    } else if (tmp == GLX_NON_CONFORMANT_CONFIG) {
        info.caveat = EGL_NON_CONFORMANT_CONFIG;
    }
    EXIT_IF_FALSE(glXGetFBConfigAttrib(
            dpy, frmt, GLX_MAX_PBUFFER_WIDTH, &info.max_pbuffer_width));
    EXIT_IF_FALSE(glXGetFBConfigAttrib(
            dpy, frmt, GLX_MAX_PBUFFER_HEIGHT, &info.max_pbuffer_height));
    EXIT_IF_FALSE(glXGetFBConfigAttrib(
            dpy, frmt, GLX_MAX_PBUFFER_HEIGHT, &info.max_pbuffer_size));

    EXIT_IF_FALSE(glXGetFBConfigAttrib(
            dpy, frmt, GLX_LEVEL, &info.frame_buffer_level));

    EXIT_IF_FALSE(glXGetFBConfigAttrib(
            dpy, frmt, GLX_SAMPLES, &info.samples_per_pixel));

    // Filter out configs that do not support RGBA
    EXIT_IF_FALSE(glXGetFBConfigAttrib(dpy, frmt, GLX_RENDER_TYPE, &tmp));
    if (!(tmp & GLX_RGBA_BIT)) {
        return;
    }
    // Filter out configs that do not support depthstencil buffers
    // For dEQP-GLES2.functional.depth_stencil_clear
    // and dEQP-GLES2.usecases.*
    if (info.depth_size == 0 || info.stencil_size == 0) {
        return;
    }

    info.frmt = new GlxPixelFormat(frmt);
    (*addConfigFunc)(addConfigOpaque, &info);
}

// Implementation of EglOS::Context based on GLX.
class GlxContext : public EglOS::Context {
public:
    explicit GlxContext(X11Display* display,
                        GLXContext context) :
        mDisplay(display),
        mContext(context) {}

    GLXContext context() const { return mContext; }

    ~GlxContext() {
        PROFILE_SLOW("~GlxContext()");
        glXDestroyContext(mDisplay, mContext);
    }

    static GLXContext contextFor(EglOS::Context* context) {
        return static_cast<GlxContext*>(context)->context();
    }
private:
    X11Display* mDisplay = nullptr;
    GLXContext mContext = nullptr;
};


// Implementation of EglOS::Display based on GLX.
class GlxDisplay : public EglOS::Display {
public:
    explicit GlxDisplay(X11Display* disp) : mDisplay(disp) {}

    virtual ~GlxDisplay() {
        PROFILE_SLOW("displayCleanup");

        for (auto it : mLivePbufs)  {
            for (auto surf : it.second) {
                glXDestroyPbuffer(mDisplay,
                        GlxSurface::drawableFor(surf));
            }
        }

        for (auto it : mFreePbufs)  {
            for (auto surf : it.second) {
                glXDestroyPbuffer(mDisplay,
                        GlxSurface::drawableFor(surf));
            }
        }

        XCloseDisplay(mDisplay);
    }

    virtual EglOS::GlesVersion getMaxGlesVersion() {
        if (!mCoreProfileSupported) {
            return EglOS::GlesVersion::ES2;
        }

        return EglOS::calcMaxESVersionFromCoreVersion(
                   mCoreMajorVersion, mCoreMinorVersion);
    }

    virtual void queryConfigs(int renderableType,
                              EglOS::AddConfigCallback* addConfigFunc,
                              void* addConfigOpaque) {
        int n;
        GLXFBConfig* frmtList = glXGetFBConfigs(mDisplay, 0, &n);
        if (frmtList) {
            mFBConfigs.assign(frmtList, frmtList + n);
            for(int i = 0; i < n; i++) {
                pixelFormatToConfig(
                        mDisplay,
                        renderableType,
                        frmtList[i],
                        addConfigFunc,
                        addConfigOpaque);
            }
            XFree(frmtList);
        }

        int glxMaj, glxMin;
        bool successQueryVersion =
            glXQueryVersion(mDisplay,
                            &glxMaj,
                            &glxMin);

        if (successQueryVersion) {
            if (glxMaj < 1 || (glxMaj >= 1 && glxMin < 4)) {
                // core profile not supported in this GLX.
                mCoreProfileSupported = false;
            } else {
                queryCoreProfileSupport();
            }
        } else {
            ERR("%s: Could not query GLX version!\n", __func__);
        }
    }

    virtual bool isValidNativeWin(EglOS::Surface* win) {
        if (!win) {
            return false;
        } else {
            return isValidNativeWin(GlxSurface::drawableFor(win));
        }
    }

    virtual bool isValidNativeWin(EGLNativeWindowType win) {
        Window root;
        int t;
        unsigned int u;
        ErrorHandler handler(mDisplay);
        if (!XGetGeometry(mDisplay, win, &root, &t, &t, &u, &u, &u, &u)) {
            return false;
        }
        return handler.getLastError() == 0;
    }

    virtual bool checkWindowPixelFormatMatch(
            EGLNativeWindowType win,
            const EglOS::PixelFormat* pixelFormat,
            unsigned int* width,
            unsigned int* height) {
        //TODO: to check what does ATI & NVIDIA enforce on win pixelformat
        unsigned int depth, configDepth, border;
        int r, g, b, x, y;
        GLXFBConfig fbconfig = GlxPixelFormat::from(pixelFormat);

        IS_SUCCESS(glXGetFBConfigAttrib(
                mDisplay, fbconfig, GLX_RED_SIZE, &r));
        IS_SUCCESS(glXGetFBConfigAttrib(
                mDisplay, fbconfig, GLX_GREEN_SIZE, &g));
        IS_SUCCESS(glXGetFBConfigAttrib(
                mDisplay, fbconfig, GLX_BLUE_SIZE, &b));
        configDepth = r + g + b;
        Window root;
        if (!XGetGeometry(
                mDisplay, win, &root, &x, &y, width, height, &border, &depth)) {
            return false;
        }
        return depth >= configDepth;
    }

    virtual emugl::SmartPtr<EglOS::Context> createContext(
            EGLint profileMask,
            const EglOS::PixelFormat* pixelFormat,
            EglOS::Context* sharedContext) {
        PROFILE_SLOW("createContext");

        bool useCoreProfile = mCoreProfileSupported &&
           (profileMask & EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR);

        ErrorHandler handler(mDisplay);

        GLXContext ctx;
        if (useCoreProfile) {
            ctx = mCreateContextAttribs(
                        mDisplay,
                        GlxPixelFormat::from(pixelFormat),
                        sharedContext ? GlxContext::contextFor(sharedContext) : NULL,
                        True /* try direct (supposed to fall back to indirect) */,
                        mCoreProfileCtxAttribs);
        } else {
            ctx = glXCreateNewContext(
                    mDisplay,
                    GlxPixelFormat::from(pixelFormat),
                    GLX_RGBA_TYPE,
                    sharedContext ? GlxContext::contextFor(sharedContext) : NULL,
                    true);
        }

        if (handler.getLastError()) {
            return NULL;
        }

        return std::make_shared<GlxContext>(mDisplay, ctx);
    }

    virtual bool destroyContext(EglOS::Context* context) {
        PROFILE_SLOW("destroyContext");
        glXDestroyContext(mDisplay, GlxContext::contextFor(context));
        return true;
    }

    void debugCountPbufs(const char* eventName, GLXFBConfig config) {
#if DEBUG_PBUF_POOL
        size_t pbufsFree = 0;
        size_t pbufsLive = 0;

        for (const auto it: mFreePbufs) {
            pbufsFree += it.second.size();
        }

        for (const auto it: mLivePbufs) {
            pbufsLive += it.second.size();
        }

        fprintf(stderr, "event: %s config: %p Pbuffer counts: free: %zu live: %zu\n",
                eventName, config, pbufsFree, pbufsLive);
#endif
    }

    virtual EglOS::Surface* createPbufferSurface(
            const EglOS::PixelFormat* pixelFormat,
            const EglOS::PbufferInfo* info) {

        android::base::AutoLock lock(mPbufLock);

        GLXFBConfig config = GlxPixelFormat::from(pixelFormat);

        debugCountPbufs("about to create", config);

        bool needPrime = false;
        if (mFreePbufs.find(config) == mFreePbufs.end()) {
            needPrime = true;
        }

        auto& freeElts = mFreePbufs[config];

        if (freeElts.size() == 0) {
            needPrime = true;
        }

        if (needPrime) {
            PROFILE_SLOW("createPbufferSurface (slow path)");
            // Double the # pbufs of this config, or create |mPbufPrimingCount|
            // of them, whichever is higher.
            int toCreate = std::max((int)mLivePbufs[config].size(),
                                    mPbufPrimingCount);
            for (int i = 0; i < toCreate; i++) {
                freeElts.push_back(createPbufferSurfaceImpl(config));
            }
        }

        PROFILE_SLOW("createPbufferSurface (fast path)");
        EglOS::Surface* surf = freeElts.back();
        freeElts.pop_back();

        auto& forLive = mLivePbufs[config];
        forLive.push_back(surf);

        return surf;
    }

    virtual bool releasePbuffer(EglOS::Surface* pb) {

        android::base::AutoLock lock(mPbufLock);

        PROFILE_SLOW("releasePbuffer");
        if (!pb) {
            return false;
        } else {
            GLXFBConfig config = GlxSurface::configFor(pb);

            debugCountPbufs("about to release", config);

            auto& frees = mFreePbufs[config];
            frees.push_back(pb);

            auto& forLive = mLivePbufs[config];
            forLive.erase(std::remove(forLive.begin(), forLive.end(), pb), forLive.end());
            return true;
        }
    }

    virtual bool makeCurrent(EglOS::Surface* read,
                             EglOS::Surface* draw,
                             EglOS::Context* context) {
        PROFILE_SLOW("makeCurrent");
        ErrorHandler handler(mDisplay);
        bool retval = false;
        if (!context && !read && !draw) {
            // unbind
            retval = glXMakeContextCurrent(mDisplay, 0, 0, NULL);
        }
        else if (context && read && draw) {
            retval = glXMakeContextCurrent(
                    mDisplay,
                    GlxSurface::drawableFor(draw),
                    GlxSurface::drawableFor(read),
                    GlxContext::contextFor(context));
            if (mSwapInterval && draw->type() == GlxSurface::SurfaceType::WINDOW) {
                android::base::AutoLock lock(mPbufLock);
                auto it = mDisabledVsyncWindows.find(draw);
                bool notPresent = it == mDisabledVsyncWindows.end();
                if (notPresent || !it->second) {
                    mSwapInterval(mDisplay, GlxSurface::drawableFor(draw), 0);
                    if (notPresent) {
                        mDisabledVsyncWindows[draw] = true;
                    } else {
                        it->second = true;
                    }
                }
            }
        }
        int err = handler.getLastError();
        return (err == 0) && retval;
    }

    virtual void swapBuffers(EglOS::Surface* srfc) {
        if (srfc) {
            glXSwapBuffers(mDisplay, GlxSurface::drawableFor(srfc));
        }
    }

private:
    using CreateContextAttribs =
        GLXContext (*)(X11Display*, GLXFBConfig, GLXContext, Bool, const int*);
    using SwapInterval =
        void (*)(X11Display*, GLXDrawable, int);

    // Returns the highest level of OpenGL core profile support in
    // this GLX implementation.
    void queryCoreProfileSupport() {
        mCoreProfileSupported = false;
        ErrorHandler handler(mDisplay);

        GlxLibrary* lib = sGlxLibrary.ptr();
        mCreateContextAttribs =
            (CreateContextAttribs)lib->findSymbol("glXCreateContextAttribsARB");
        mSwapInterval =
            (SwapInterval)lib->findSymbol("glXSwapIntervalEXT");

        if (!mCreateContextAttribs || mFBConfigs.size() == 0) return;

        if (!mSwapInterval) {
            fprintf(stderr, "%s: swap interval not found\n", __func__);
        }

        // Ascending index order of context attribs :
        // decreasing GL major/minor version
        GLXContext testContext = nullptr;

        for (int i = 0; i < getNumCoreProfileCtxAttribs(); i++) {
            const int* attribs = getCoreProfileCtxAttribs(i);
            testContext =
                mCreateContextAttribs(
                        mDisplay, mFBConfigs[0],
                        nullptr /* no shared context */,
                        True /* try direct (supposed to fall back to indirect) */,
                        attribs);

            if (testContext) {
                mCoreProfileSupported = true;
                mCoreProfileCtxAttribs = attribs;
                getCoreProfileCtxAttribsVersion(
                    attribs, &mCoreMajorVersion, &mCoreMinorVersion);
                glXDestroyContext(mDisplay, testContext);
                return;
            }
        }
    }

    EglOS::Surface* createPbufferSurfaceImpl(GLXFBConfig config) {
        // we never care about width or height, since we just use
        // opengl fbos anyway.
        static const int pbufferImplAttribs[] = {
            GLX_PBUFFER_WIDTH, 1,
            GLX_PBUFFER_HEIGHT, 1,
            GLX_LARGEST_PBUFFER, 0,
            None
        };

        GLXPbuffer pb;
        pb = glXCreatePbuffer(
                mDisplay,
                config,
                pbufferImplAttribs);

        return new GlxSurface(pb, config, GlxSurface::PBUFFER);
    }

    CreateContextAttribs mCreateContextAttribs = nullptr;
    SwapInterval mSwapInterval = nullptr;

    bool mCoreProfileSupported = false;
    int mCoreMajorVersion = 4;
    int mCoreMinorVersion = 5;
    const int* mCoreProfileCtxAttribs = nullptr;

    X11Display* mDisplay = nullptr;
    std::vector<GLXFBConfig> mFBConfigs;
    std::unordered_map<GLXFBConfig, std::vector<EglOS::Surface* > > mFreePbufs;
    std::unordered_map<GLXFBConfig, std::vector<EglOS::Surface* > > mLivePbufs;
    int mPbufPrimingCount = 8;
    android::base::Lock mPbufLock;
    std::unordered_map<EglOS::Surface*, bool> mDisabledVsyncWindows;
};

class GlxEngine : public EglOS::Engine {
public:
    virtual EglOS::Display* getDefaultDisplay() {
        return new GlxDisplay(XOpenDisplay(0));
    }

    virtual GlLibrary* getGlLibrary() {
        return sGlxLibrary.ptr();
    }

    virtual EglOS::Surface* createWindowSurface(EglOS::PixelFormat* pf,
                                                EGLNativeWindowType wnd) {
        return new GlxSurface(wnd, 0, GlxSurface::WINDOW);
    }
};

emugl::LazyInstance<GlxEngine> sHostEngine = LAZY_INSTANCE_INIT;

}  // namespace

// static
EglOS::Engine* EglOS::Engine::getHostInstance() {
    return sHostEngine.ptr();
}
