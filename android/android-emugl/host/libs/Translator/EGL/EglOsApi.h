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
#ifndef EGL_OS_API_H
#define EGL_OS_API_H

#include "aemu/base/Compiler.h"

#include "emugl/common/smart_ptr.h"

#include <EGL/egl.h>

#define PBUFFER_MAX_WIDTH  32767
#define PBUFFER_MAX_HEIGHT 32767
#define PBUFFER_MAX_PIXELS (PBUFFER_MAX_WIDTH * PBUFFER_MAX_HEIGHT)

class GlLibrary;

namespace EglOS {

// This header contains declaration used to abstract the underlying
// desktop GL library (or equivalent) that is being used by our EGL
// and GLES translation libraries.

// Use EglOS::Engine::getHostInstance() to retrieve an instance of the
// EglOS::Engine interface that matches the host display system.
//
// Alternate renderers (e.g. software-based Mesa) can also implement
// their own engine.

// Base class used to wrap various GL Surface types.
class Surface {
public:
    typedef enum {
        WINDOW = 0,
        PBUFFER = 1,
    } SurfaceType;

    explicit Surface(SurfaceType type) : mType(type) {}

    SurfaceType type() const { return mType; }

protected:
    SurfaceType mType;
};

// An interface class for engine-specific implementation of a GL context.
class Context {
public:
    Context(bool coreProfile = false) : mCoreProfile(coreProfile) {}
    bool isCoreProfile() const {
        return mCoreProfile;
    }

    // as of now, only osx has this non-nullptr, needed by media decoder
    virtual void* lowLevelContext() { return nullptr; }
    virtual void* getNative() { return nullptr; }

protected:
    ~Context() = default;
private:
    bool mCoreProfile = false;
};

// Base class used to wrap engine-specific pixel format descriptors.
class PixelFormat {
public:
    PixelFormat() {}

    virtual ~PixelFormat() {}

    virtual PixelFormat* clone() = 0;
};

// Small structure used to describe the properties of an engine-specific
// config.
struct ConfigInfo {
    EGLint red_size;
    EGLint green_size;
    EGLint blue_size;
    EGLint alpha_size;
    EGLenum caveat;
    EGLint depth_size;
    EGLint frame_buffer_level;
    EGLint max_pbuffer_width;
    EGLint max_pbuffer_height;
    EGLint max_pbuffer_size;
    EGLBoolean native_renderable;
    EGLint renderable_type;
    EGLint native_visual_id;
    EGLint native_visual_type;
    EGLint samples_per_pixel;
    EGLint stencil_size;
    EGLint surface_type;
    EGLenum transparent_type;
    EGLint trans_red_val;
    EGLint trans_green_val;
    EGLint trans_blue_val;
    EGLBoolean recordable_android;
    PixelFormat* frmt;
};

// A callback function type used with Display::queryConfig() to report to the
// caller a new host EGLConfig.
// |opaque| is an opaque value passed to queryConfig().
// All other parameters are config attributes.
// Note that ownership of |frmt| is transfered to the callback.
typedef void (AddConfigCallback)(void* opaque, const ConfigInfo* configInfo);

// Pbuffer description.
// |width| and |height| are its dimensions.
// |largest| is set to ask the largest pixek buffer (see GLX_LARGEST_PBUFFER).
// |format| is one of EGL_TEXTURE_RGB or EGL_TEXTURE_RGBA
// |target| is one of EGL_TEXTURE_2D or EGL_NO_TEXTURE.
// |hasMipmap| is true if the Pbuffer has mipmaps.
struct PbufferInfo {
    EGLint width;
    EGLint height;
    EGLint largest;
    EGLint format;
    EGLint target;
    EGLint hasMipmap;
};

enum class GlesVersion {
    ES2 = 0,
    ES30 = 1,
    ES31 = 2,
    ES32 = 3,
};

inline GlesVersion calcMaxESVersionFromCoreVersion(int coreMajor, int coreMinor) {
    switch (coreMajor) {
        case 3:
            return coreMinor > 1 ? EglOS::GlesVersion::ES30 : EglOS::GlesVersion::ES2;
        case 4:
            // 4.3 core has all the entry points we need, but we want 4.5 core for
            // ARB_ES31_compatibility to avoid shader translation (for now. TODO:
            // translate ESSL 310 to 4.3 shaders)
            return coreMinor > 4 ? EglOS::GlesVersion::ES31 : EglOS::GlesVersion::ES30;
        default:
            return EglOS::GlesVersion::ES2;
    }
}

// A class to model the engine-specific implementation of a GL display
// connection.
class Display {
public:
    Display() = default;
    virtual ~Display() {}

    virtual GlesVersion getMaxGlesVersion() = 0;
    virtual const char* getExtensionString() { return ""; }

    virtual void queryConfigs(int renderableType,
                              AddConfigCallback* addConfigFunc,
                              void* addConfigOpaque) = 0;

    virtual bool isValidNativeWin(Surface* win) = 0;
    virtual bool isValidNativeWin(EGLNativeWindowType win) = 0;

    virtual bool checkWindowPixelFormatMatch(EGLNativeWindowType win,
                                             const PixelFormat* pixelFormat,
                                             unsigned int* width,
                                             unsigned int* height) = 0;

    virtual emugl::SmartPtr<Context> createContext(
            EGLint profileMask,
            const PixelFormat* pixelFormat,
            Context* sharedContext) = 0;

    virtual Surface* createPbufferSurface(
            const PixelFormat* pixelFormat, const PbufferInfo* info) = 0;

    virtual EGLImage createImage(
            EGLDisplay,
            EGLContext,
            EGLenum,
            EGLClientBuffer,
            const EGLint* attribs) {
        return (EGLImage)0;
    }

    virtual EGLBoolean destroyImage(
            EGLDisplay,
            EGLImage) { return EGL_FALSE; }

    virtual EGLDisplay getNative() { return (EGLDisplay)0; }

    virtual bool releasePbuffer(Surface* pb) = 0;

    virtual bool makeCurrent(Surface* read,
                             Surface* draw,
                             Context* context) = 0;

    virtual void swapBuffers(Surface* srfc) = 0;

    DISALLOW_COPY_AND_ASSIGN(Display);
};

// An interface class to model a specific underlying GL graphics subsystem
// or engine. Use getHost() to retrieve the implementation for the current
// host.
class Engine {
public:
    Engine() = default;
    virtual ~Engine() {}

    // Return a Display instance to the default display / window.
    virtual Display* getDefaultDisplay() = 0;

    // Return to engine-specific implementation of GlLibrary.
    virtual GlLibrary* getGlLibrary() = 0;

    // Create a new window surface. |wnd| is a host-specific window handle
    // (e.g. a Windows HWND). A software renderer would always return NULL
    // here.
    virtual Surface* createWindowSurface(PixelFormat* cfg,
                                         EGLNativeWindowType wnd) = 0;

    // Retrieve the implementation for the current host. This can be called
    // multiple times, and will initialize the engine on first call.
    static Engine* getHostInstance();
};

// getEgl2EglHostInstance returns a host instance that is used to mount
// EGL/GLES translator on top of another EGL/GLES library
Engine* getEgl2EglHostInstance();

}  // namespace EglOS

#endif  // EGL_OS_API_H
