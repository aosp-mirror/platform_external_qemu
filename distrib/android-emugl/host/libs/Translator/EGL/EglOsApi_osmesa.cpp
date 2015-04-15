// Copyright (C) 2015 The Android Open Source Project
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

#include "EglOsApi_osmesa.h"

#include "OpenglCodecCommon/ErrorLog.h"

#include "emugl/common/lazy_instance.h"
#include "emugl/common/shared_library.h"

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <stdlib.h>
#include <string.h>

// WARNING: THIS IS PROBABLY VERY BROKEN AT THIS POINT

namespace {

using emugl::SharedLibrary;

// OSMesa declarations, these come from <GL/osmesa.h> adapted to lazily
// load and use the library.

// Passed as |format| values to OSMesaCreateContext() and
// OSMesaCreateContextExt()
#define OSMESA_RGBA GL_RGBA
#define OSMESA_RGB  GL_RGB

#ifdef _WIN32
#  define OSMESA_APICALL  __declspec(dllimport)
#  define OSMESA_APIENTRY __stdcall
#else
#  define OSMESA_APICALL
#  define OSMESA_APIENTRY
#endif

// A context object backed by the OSMesa library.
typedef struct osmesa_context* OSMesaContext;

// A macro used to list the declarations of all OSMesa functions exported
// by libosmesa.so that we want to use. |X| is a macro that takes three
// parameters which are:
//   - The function return type.
//   - The function name.
//   - The function signature.
#define OSMESA_LIST_FUNCTIONS(X)                                \
    X(OSMesaContext,OSMesaCreateContextExt,                     \
            (GLenum format, GLint depthBits, GLint stencilBits, \
             GLint accumBits,OSMesaContext sharedlist))         \
    X(void,OSMesaDestroyContext,(OSMesaContext context))        \
    X(GLboolean,OSMesaMakeCurrent,                              \
            (OSMesaContext ctx, void* buffer, GLenum type,      \
             GLsizei width, GLsizei height))                    \

// osmesa_funcs is a structure containing pointers to OSMesa functions.
#define OSMESA_DEFINE_FUNC_POINTER(rettype,name,signature) \
    OSMESA_APICALL rettype (OSMESA_APIENTRY *name) signature;

#define OSMESA_LOAD_FUNC_POINTER(rettype,name,signature) \
    this->name = reinterpret_cast<OSMESA_APICALL rettype(OSMESA_APIENTRY *)signature>(  \
            mLib->findSymbol(#name));                      \
    if (!this->name) goto EXIT;

// OSMesaFuncs is a structure that contains pointers to the OSMesa functions
// exported from libosmesa.so. Call the init() function to initialize it, then
// use each pointer directly to make function calls into the library.
class OSMesaFuncs {
public:
    OSMesaFuncs() : mOk(false), mLib(NULL) {
#ifdef _WIN32
        const char kLibName[] = "osmesa";
#else
        const char kLibName[] = "libosmesa";
#endif
        mLib = SharedLibrary::open(kLibName);
        if (mLib) {
            OSMESA_LIST_FUNCTIONS(OSMESA_LOAD_FUNC_POINTER)
            mOk = true;
        EXIT:
            ;
        }
    }

    ~OSMesaFuncs() {
        delete mLib;
    }

    bool ok() const { return mOk; }

    // Define function pointers.
    OSMESA_LIST_FUNCTIONS(OSMESA_DEFINE_FUNC_POINTER)

private:
    bool mOk;
    SharedLibrary* mLib;
};

emugl::LazyInstance<OSMesaFuncs> sOSMesaFuncs = LAZY_INSTANCE_INIT;

const OSMesaFuncs* osmesa() {
    return sOSMesaFuncs.ptr();
}

// A small structure used to describe a supported pixel format.
// |pixelFormat| is the pixel format, using OSMesa constants, i.e.
// OSMESA_RGB or OSMESA_RGBA.
// |depthSize| is the depth size in bits. 0 for none.
// |stencilSize| is the stencil size in bits. 0 for none.
struct PixelConfig {
    int pixelFormat;
    int depthBits;
    int stencilBits;
};

// A list of supported pixel configurations.
static const PixelConfig kPixelConfigs[] = {
    { OSMESA_RGB, 24, 8 },
    { OSMESA_RGBA, 24, 8 },
    { OSMESA_RGB, 0, 0 },
    { OSMESA_RGBA, 0, 0 },
};

static const int kPixelConfigsLen =
        sizeof(kPixelConfigs) / sizeof(kPixelConfigs[0]);

// EglOS::PixelFormat implementation for OSMesa, which simply wraps a
// PixelConfig pointer.
class OffscreenMesaPixelFormat : public EglOS::PixelFormat {
public:
    explicit OffscreenMesaPixelFormat(const PixelConfig* pixelConfig) :
            mPixelConfig(pixelConfig) {}

    virtual EglOS::PixelFormat* clone() {
        return new OffscreenMesaPixelFormat(mPixelConfig);
    }

    const PixelConfig* pixelConfig() const { return mPixelConfig; }

    static const OffscreenMesaPixelFormat* from(const EglOS::PixelFormat* f) {
        return reinterpret_cast<const OffscreenMesaPixelFormat*>(f);
    }

private:
    OffscreenMesaPixelFormat();
    OffscreenMesaPixelFormat(const OffscreenMesaPixelFormat&);

    const PixelConfig* mPixelConfig;
};

void pixelFormatToConfig(int index,
                         int renderableType,
                         const PixelConfig* pixelConfig,
                         EglOS::AddConfigCallback addConfigFunc,
                         void* addConfigOpaque) {
    EglOS::ConfigInfo info;
    memset(&info, 0, sizeof(info));

    // Can't really implement EGL_WINDOW_BIT here!
    info.surface_type |= EGL_PBUFFER_BIT;
    info.native_visual_id = 0;
    info.native_visual_type = EGL_NONE;
    info.caveat = EGL_NONE;
    info.renderable_type = renderableType;
    info.native_renderable = EGL_FALSE;
    info.max_pbuffer_width = PBUFFER_MAX_WIDTH;
    info.max_pbuffer_height = PBUFFER_MAX_HEIGHT;
    info.max_pbuffer_size = PBUFFER_MAX_PIXELS;
    info.frame_buffer_level = 0;
    info.trans_red_val = 0;
    info.trans_green_val = 0;
    info.trans_blue_val = 0;
    info.transparent_type = EGL_NONE;
    info.samples_per_pixel = 0;

    switch (pixelConfig->pixelFormat) {
    case OSMESA_RGB:
        info.red_size = 8;
        info.green_size = 8;
        info.blue_size = 8;
        info.alpha_size = 0;
        break;
    case OSMESA_RGBA:
        info.red_size = 8;
        info.green_size = 8;
        info.blue_size = 8;
        info.alpha_size = 8;
        break;
    default:
        return;
    }

    info.stencil_size = pixelConfig->stencilBits;
    info.depth_size = pixelConfig->depthBits;
    info.config_id = (EGLint) index;
    info.frmt = new OffscreenMesaPixelFormat(pixelConfig);

    (*addConfigFunc)(addConfigOpaque, &info);
}

// EglOS::Surface based on OSMesa.
class OffscreenMesaSurface : public EglOS::Surface {
public:
    OffscreenMesaSurface(int format, int width, int height) :
            EglOS::Surface(PBUFFER),
            mFormat(format),
            mWidth(width),
            mHeight(height),
            mData(NULL) {
        int pitch = width * 4;
        mData = static_cast<uint8_t*>(::calloc(pitch, height));
    }

    ~OffscreenMesaSurface() {
        free(mData);
    }

    int format() const { return mFormat; }
    int width() const { return mWidth; }
    int height() const { return mHeight; }
    uint8_t* data() const { return mData; }

    static OffscreenMesaSurface* from(EglOS::Surface* s) {
        return static_cast<OffscreenMesaSurface*>(s);
    }

private:
    int mFormat;
    int mWidth;
    int mHeight;
    uint8_t* mData;
};

// ElgOS::Context implementation based on OSMesa.
class OffscreenMesaContext : public EglOS::Context {
public:
    OffscreenMesaContext(const PixelConfig* pixelConfig,
                         OffscreenMesaContext* shareList) {
        mContext = osmesa()->OSMesaCreateContextExt(
                pixelConfig->pixelFormat,
                pixelConfig->depthBits,
                pixelConfig->stencilBits,
                0,
                shareList ? shareList->osmesaContext() : NULL);
    }

    virtual ~OffscreenMesaContext() {
        if (mContext) {
            osmesa()->OSMesaDestroyContext(mContext);
        }
    }

    OSMesaContext osmesaContext() const { return mContext; }

    static OffscreenMesaContext* from(EglOS::Context* c) {
        return reinterpret_cast<OffscreenMesaContext*>(c);
    }

private:
    OSMesaContext mContext;
};

// An implementation of EglOS::Display interface backed by OSMesa.
class OffscreenMesaDisplay : public EglOS::Display {
public:
    virtual bool release() {
        // TODO(digit)
        return true;
    }

    virtual void queryConfigs(int renderableType,
                              EglOS::AddConfigCallback* addConfigFunc,
                              void* addConfigOpaque) {
        for (int n = 0; n < kPixelConfigsLen; ++n) {
            pixelFormatToConfig(n,
                                renderableType,
                                &kPixelConfigs[n],
                                addConfigFunc,
                                addConfigOpaque);
        }
    }

    virtual bool isValidNativeWin(EglOS::Surface* win) {
        return win->type() == EglOS::Surface::WINDOW;
    }

    virtual bool isValidNativeWin(EGLNativeWindowType win) {
        return false;
    }

    virtual bool isValidNativePixmap(EglOS::Surface* pix) {
        return pix->type() == EglOS::Surface::PIXMAP;
    }

    virtual bool checkWindowPixelFormatMatch(
            EGLNativeWindowType win,
            const EglOS::PixelFormat* pixelFormat,
            unsigned int* width,
            unsigned int* height) {
        return false;
    }

    virtual bool checkPixmapPixelFormatMatch(
            EGLNativePixmapType pix,
            const EglOS::PixelFormat* pixelFormat,
            unsigned int* width,
            unsigned int* height) {
        return false;
    }

    virtual EglOS::Context* createContext(
            const EglOS::PixelFormat* pixelFormat,
            EglOS::Context* sharedContext) {
        if (!pixelFormat) {
            return NULL;
        }
        const PixelConfig* pixelConfig =
                OffscreenMesaPixelFormat::from(pixelFormat)->pixelConfig();

        OffscreenMesaContext* shared = NULL;
        if (sharedContext) {
            shared = OffscreenMesaContext::from(sharedContext);
        }
        return new OffscreenMesaContext(pixelConfig, shared);
    }

    virtual bool destroyContext(EglOS::Context* context) {
        delete context;
        return true;
    }

    virtual EglOS::Surface* createPbufferSurface(
            const EglOS::PixelFormat* pixelFormat,
            const EglOS::PbufferInfo* info) {
        if (!pixelFormat || !info) {
            return NULL;
        }
        // Determine OSMesa format of surface pixels.
        const PixelConfig* pixelConfig =
                OffscreenMesaPixelFormat::from(pixelFormat)->pixelConfig();

        return new OffscreenMesaSurface(
                pixelConfig->pixelFormat,
                info->width,
                info->height);
    }

    virtual bool releasePbuffer(EglOS::Surface* pb) {
        if (!pb) {
            return false;
        }
        return true;
    }

    virtual bool makeCurrent(EglOS::Surface* read,
                             EglOS::Surface* draw,
                             EglOS::Context* context) {
        // TODO(digit): Determine what to do here.

        // OSMesa doesn't allow one to unset the current context for now,
        // so just return.
        if (!read && !draw && !context) {
            return true;
        }

        if (!read || !draw || !context) {
            return false;
        }

        // TODO(digit): There is a big issue where OSMesaMakeCurrent()
        // always assumes that the |read| and |draw| surfaces are the same :-(
        if (read != draw) {
            ERR("%s: read (%p) != draw (%p)\n", __FUNCTION__, read, draw);
            return false;
        }

        OffscreenMesaSurface* osmesaRead = OffscreenMesaSurface::from(read);

        GLboolean result = osmesa()->OSMesaMakeCurrent(
                OffscreenMesaContext::from(context)->osmesaContext(),
                static_cast<void*>(osmesaRead->data()),
                GL_UNSIGNED_BYTE,
                osmesaRead->width(),
                osmesaRead->height());

        return (result == GL_TRUE);
    }

    virtual void swapBuffers(EglOS::Surface* srfc) {
        // TODO(digit): Determine what to do here.
    }

    virtual void swapInterval(EglOS::Surface* win, int interval) {
        // TODO(digit): Determine what to do here.
    }
};

// An implementation of the EglOS::Engine interface backed by OSMesa.
class OSMesaEngine : public EglOS::Engine {
public:
    virtual EglOS::Display* getDefaultDisplay() {
        return new OffscreenMesaDisplay();
    }

    virtual EglOS::Display* getInternalDisplay(EGLNativeDisplayType dpy) {
        // No support for native host display types.
        return NULL;
    }

    virtual EglOS::Surface* createWindowSurface(EGLNativeWindowType wnd) {
        // Always return NULL since OSMesa doesn't support displaying
        // anything on the host graphics system.
        return NULL;
    }

    virtual EglOS::Surface* createPixmapSurface(EGLNativePixmapType pix) {
        // Always return NULL since OSMesa doesn't support host pixmaps.
        return NULL;
    }

    virtual void wait() {}
};

emugl::LazyInstance<OSMesaEngine> sOSMesaEngine = LAZY_INSTANCE_INIT;

}  // namespace

EglOS::Engine* EglOS::getOSMesaEngineInstance() {
    // First try to load the offscreen mesa library, if this fail, return.
    if (!osmesa()->ok()) {
        return NULL;
    }
    return sOSMesaEngine.ptr();
}

