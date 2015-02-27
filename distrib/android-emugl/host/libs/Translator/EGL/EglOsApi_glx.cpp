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

#include "EglConfig.h"

#include "emugl/common/lazy_instance.h"
#include "emugl/common/mutex.h"

#include <string.h>
#include <X11/Xlib.h>
#include <GL/glx.h>

namespace {

typedef Display X11Display;

class ErrorHandler{
public:
    ErrorHandler(EGLNativeDisplayType dpy);
    ~ErrorHandler();
    int getLastError() const { return s_lastErrorCode; }

private:
    static int s_lastErrorCode;
    int (*m_oldErrorHandler)(Display *, XErrorEvent *);
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
        if(a != Success) return 0;

// Implementation of EglOS::PixelFormat based on GLX.
class GlxPixelFormat : public EglOS::PixelFormat {
public:
    explicit GlxPixelFormat(GLXFBConfig fbconfig) : mFbConfig(fbconfig) {}

    virtual EglOS::PixelFormat* clone() {
        return new GlxPixelFormat(mFbConfig);
    }

    GLXFBConfig fbConfig() const { return mFbConfig; }

    static GLXFBConfig from(EglOS::PixelFormat* f) {
        return static_cast<GlxPixelFormat*>(f)->fbConfig();
    }

private:
    GLXFBConfig mFbConfig;
};

EglConfig* pixelFormatToConfig(EGLNativeDisplayType dpy,
                               int renderableType,
                               GLXFBConfig frmt) {
    int  bSize, red, green, blue, alpha, depth, stencil;
    int  supportedSurfaces, visualType, visualId;
    int  caveat, transparentType, samples;
    int  tRed = 0, tGreen = 0, tBlue = 0;
    int  pMaxWidth, pMaxHeight, pMaxPixels;
    int  tmp;
    int  configId, level, renderable;
    int  doubleBuffer;

    IS_SUCCESS(glXGetFBConfigAttrib(dpy, frmt, GLX_TRANSPARENT_TYPE, &tmp));
    if (tmp == GLX_TRANSPARENT_INDEX) {
        return NULL; // not supporting transparent index
    } else if (tmp == GLX_NONE) {
        transparentType = EGL_NONE;
    } else {
        transparentType = EGL_TRANSPARENT_RGB;

        IS_SUCCESS(glXGetFBConfigAttrib(
                dpy, frmt, GLX_TRANSPARENT_RED_VALUE, &tRed));
        IS_SUCCESS(glXGetFBConfigAttrib(
                dpy, frmt, GLX_TRANSPARENT_GREEN_VALUE, &tGreen));
        IS_SUCCESS(glXGetFBConfigAttrib(
                dpy, frmt, GLX_TRANSPARENT_BLUE_VALUE, &tBlue));
    }

    //
    // filter out single buffer configurations
    //
    IS_SUCCESS(glXGetFBConfigAttrib(
            dpy, frmt, GLX_DOUBLEBUFFER, &doubleBuffer));
    if (!doubleBuffer) {
        return NULL;
    }

    IS_SUCCESS(glXGetFBConfigAttrib(dpy ,frmt, GLX_BUFFER_SIZE, &bSize));
    IS_SUCCESS(glXGetFBConfigAttrib(dpy ,frmt, GLX_RED_SIZE, &red));
    IS_SUCCESS(glXGetFBConfigAttrib(dpy ,frmt, GLX_GREEN_SIZE, &green));
    IS_SUCCESS(glXGetFBConfigAttrib(dpy ,frmt, GLX_BLUE_SIZE, &blue));
    IS_SUCCESS(glXGetFBConfigAttrib(dpy ,frmt, GLX_ALPHA_SIZE, &alpha));
    IS_SUCCESS(glXGetFBConfigAttrib(dpy ,frmt, GLX_DEPTH_SIZE, &depth));
    IS_SUCCESS(glXGetFBConfigAttrib(dpy ,frmt, GLX_STENCIL_SIZE, &stencil));


    IS_SUCCESS(glXGetFBConfigAttrib(dpy, frmt, GLX_X_RENDERABLE, &renderable));

    IS_SUCCESS(glXGetFBConfigAttrib(
            dpy, frmt, GLX_X_VISUAL_TYPE, &visualType));

    IS_SUCCESS(glXGetFBConfigAttrib(dpy, frmt, GLX_VISUAL_ID, &visualId));

    //supported surfaces types
    IS_SUCCESS(glXGetFBConfigAttrib(dpy, frmt, GLX_DRAWABLE_TYPE, &tmp));
    supportedSurfaces = 0;
    if (tmp & GLX_WINDOW_BIT && visualId != 0) {
        supportedSurfaces |= EGL_WINDOW_BIT;
    } else {
        visualId = 0;
        visualType = EGL_NONE;
    }
    if (tmp & GLX_PBUFFER_BIT) {
        supportedSurfaces |= EGL_PBUFFER_BIT;
    }

    caveat = 0;
    IS_SUCCESS(glXGetFBConfigAttrib(dpy, frmt, GLX_CONFIG_CAVEAT, &tmp));
    if (tmp == GLX_NONE) {
        caveat = EGL_NONE;
    } else if (tmp == GLX_SLOW_CONFIG) {
        caveat = EGL_SLOW_CONFIG;
    } else if (tmp == GLX_NON_CONFORMANT_CONFIG) {
        caveat = EGL_NON_CONFORMANT_CONFIG;
    }
    IS_SUCCESS(glXGetFBConfigAttrib(
            dpy, frmt, GLX_MAX_PBUFFER_WIDTH, &pMaxWidth));
    IS_SUCCESS(glXGetFBConfigAttrib(
            dpy, frmt, GLX_MAX_PBUFFER_HEIGHT, &pMaxHeight));
    IS_SUCCESS(glXGetFBConfigAttrib(
            dpy, frmt, GLX_MAX_PBUFFER_HEIGHT, &pMaxPixels));

    IS_SUCCESS(glXGetFBConfigAttrib(dpy, frmt, GLX_LEVEL, &level));
    IS_SUCCESS(glXGetFBConfigAttrib(dpy, frmt, GLX_FBCONFIG_ID, &configId));
    IS_SUCCESS(glXGetFBConfigAttrib(dpy, frmt, GLX_SAMPLES, &samples));
    //Filter out configs that does not support RGBA
    IS_SUCCESS(glXGetFBConfigAttrib(dpy, frmt, GLX_RENDER_TYPE, &tmp));
    if (!(tmp & GLX_RGBA_BIT)) {
        return NULL;
    }

    return new EglConfig(
            red,
            green,
            blue,
            alpha,
            caveat,
            configId,
            depth,
            level,
            pMaxWidth,
            pMaxHeight,
            pMaxPixels,
            renderable,
            renderableType,
            visualId,
            visualType,
            samples,
            stencil,
            supportedSurfaces,
            transparentType,
            tRed,
            tGreen,
            tBlue,
            new GlxPixelFormat(frmt));
}

// Implementation of EglOS::Surface based on GLX.
class GlxSurface : public EglOS::Surface {
public:
    GlxSurface(GLXDrawable drawable, SurfaceType type) :
            Surface(type), mDrawable(drawable) {}

    GLXDrawable drawable() const { return mDrawable; }

    // Helper routine to down-cast an EglOS::Surface and extract
    // its drawable.
    static GLXDrawable drawableFor(EglOS::Surface* surface) {
        return static_cast<GlxSurface*>(surface)->drawable();
    }

private:
    GLXDrawable mDrawable;
};

// Implementation of EglOS::Context based on GLX.
class GlxContext : public EglOS::Context {
public:
    explicit GlxContext(GLXContext context) : mContext(context) {}

    GLXContext context() const { return mContext; }

    static GLXContext contextFor(EglOS::Context* context) {
        return static_cast<GlxContext*>(context)->context();
    }
private:
    GLXContext mContext;
};

// Implementation of EglOS::Display based on GLX.
class GlxDisplay : public EglOS::Display {
public:
    explicit GlxDisplay(X11Display* disp) : mDisplay(disp) {}

    virtual bool release() {
        return XCloseDisplay(mDisplay);
    }

    virtual void queryConfigs(int renderableType, ConfigsList& listOut) {
        int n;
        GLXFBConfig* frmtList = glXGetFBConfigs(mDisplay, 0, &n);
        for(int i = 0; i < n; i++) {
            EglConfig* conf = pixelFormatToConfig(
                    mDisplay, renderableType, frmtList[i]);
            if(conf) {
                listOut.push_back(conf);
            }
        }
        XFree(frmtList);
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

    virtual bool isValidNativePixmap(EglOS::Surface* pix) {
        Window root;
        int t;
        unsigned int u;
        ErrorHandler handler(mDisplay);
        GLXDrawable surface = pix ? GlxSurface::drawableFor(pix) : 0;
        if (!XGetGeometry(mDisplay, surface, &root, &t, &t, &u, &u, &u, &u)) {
            return false;
        }
        return handler.getLastError() == 0;
    }

    virtual bool checkWindowPixelFormatMatch(EGLNativeWindowType win,
                                             const EglConfig* cfg,
                                             unsigned int* width,
                                             unsigned int* height) {
        //TODO: to check what does ATI & NVIDIA enforce on win pixelformat
        unsigned int depth, configDepth, border;
        int r, g, b, x, y;
        GLXFBConfig fbconfig = GlxPixelFormat::from(cfg->nativeFormat());

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

    virtual bool checkPixmapPixelFormatMatch(EGLNativePixmapType pix,
                                             const EglConfig* cfg,
                                             unsigned int* width,
                                             unsigned int* height) {
        unsigned int depth, configDepth, border;
        int r, g, b, x, y;
        GLXFBConfig fbconfig = GlxPixelFormat::from(cfg->nativeFormat());

        IS_SUCCESS(glXGetFBConfigAttrib(
                mDisplay, fbconfig, GLX_RED_SIZE, &r));
        IS_SUCCESS(glXGetFBConfigAttrib(
                mDisplay, fbconfig, GLX_GREEN_SIZE, &g));
        IS_SUCCESS(glXGetFBConfigAttrib(
                mDisplay, fbconfig, GLX_BLUE_SIZE, &b));
        configDepth = r + g + b;
        Window root;
        if (!XGetGeometry(
                mDisplay, pix, &root, &x, &y, width, height, &border, &depth)) {
            return false;
        }
        return depth >= configDepth;
    }

    virtual EglOS::Context* createContext(
            const EglConfig* config, EglOS::Context* sharedContext) {
        ErrorHandler handler(mDisplay);
        GLXContext ctx = glXCreateNewContext(
                mDisplay,
                GlxPixelFormat::from(config->nativeFormat()),
                GLX_RGBA_TYPE,
                sharedContext ? GlxContext::contextFor(sharedContext) : NULL,
                true);

        if (handler.getLastError()) {
            return NULL;
        }

        return new GlxContext(ctx);
    }

    virtual bool destroyContext(EglOS::Context* context) {
        glXDestroyContext(mDisplay, GlxContext::contextFor(context));
        return true;
    }

    virtual EglOS::Surface* createPbufferSurface(
            const EglConfig* config, const EglOS::PbufferInfo* info) {
        const int attribs[] = {
            GLX_PBUFFER_WIDTH, info->width,
            GLX_PBUFFER_HEIGHT, info->height,
            GLX_LARGEST_PBUFFER, info->largest,
            None
        };
        GLXPbuffer pb = glXCreatePbuffer(
                mDisplay,
                GlxPixelFormat::from(config->nativeFormat()),
                attribs);
        return pb ? new GlxSurface(pb, GlxSurface::PBUFFER) : NULL;
    }

    virtual bool releasePbuffer(EglOS::Surface* pb) {
        if (!pb) {
            return false;
        } else {
            glXDestroyPbuffer(mDisplay, GlxSurface::drawableFor(pb));
            return true;
        }
    }

    virtual bool makeCurrent(EglOS::Surface* read,
                             EglOS::Surface* draw,
                             EglOS::Context* context) {
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
        }
        return (handler.getLastError() == 0) && retval;
    }

    virtual void swapBuffers(EglOS::Surface* srfc) {
        if (srfc) {
            glXSwapBuffers(mDisplay, GlxSurface::drawableFor(srfc));
        }
    }

    virtual void swapInterval(EglOS::Surface* win, int interval) {
        const char* extensions = glXQueryExtensionsString(
                mDisplay, DefaultScreen(mDisplay));
        typedef void (*GLXSWAPINTERVALEXT)(X11Display*,GLXDrawable,int);
        GLXSWAPINTERVALEXT glXSwapIntervalEXT = NULL;

        if(strstr(extensions,"EXT_swap_control")) {
            glXSwapIntervalEXT = (GLXSWAPINTERVALEXT)glXGetProcAddress(
                    (const GLubyte*)"glXSwapIntervalEXT");
        }
        if (glXSwapIntervalEXT && win) {
            glXSwapIntervalEXT(mDisplay,
                               GlxSurface::drawableFor(win),
                               interval);
        }
    }

private:
    X11Display* mDisplay;
};

class GlxEngine : public EglOS::Engine {
public:
    virtual EglOS::Display* getDefaultDisplay() {
        return new GlxDisplay(XOpenDisplay(0));
    }

    virtual EglOS::Display* getInternalDisplay(EGLNativeDisplayType dpy) {
        return new GlxDisplay(dpy);
    }

    virtual EglOS::Surface* createWindowSurface(EGLNativeWindowType wnd) {
        return new GlxSurface(wnd, GlxSurface::WINDOW);
    }

    virtual EglOS::Surface* createPixmapSurface(EGLNativePixmapType pix) {
        return new GlxSurface(pix, GlxSurface::PIXMAP);
    }

    virtual void wait() {
        glXWaitX();
    }
};

emugl::LazyInstance<GlxEngine> sHostEngine = LAZY_INSTANCE_INIT;

}  // namespace

// static
EglOS::Engine* EglOS::Engine::getHostInstance() {
    return sHostEngine.ptr();
}
