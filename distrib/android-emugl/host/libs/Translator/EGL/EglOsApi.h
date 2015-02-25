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

#include "EglInternal.h"
#include "EglConfig.h"

#include <EGL/egl.h>

#define PBUFFER_MAX_WIDTH  32767
#define PBUFFER_MAX_HEIGHT 32767
#define PBUFFER_MAX_PIXELS 32767*32767

namespace EglOS {

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

// A class to model the engine-specific implementation of an EGL display
// connection.
class Display {
public:
    Display() {}
    virtual ~Display() {}

    virtual bool release() = 0;

    virtual void queryConfigs(int renderableType, ConfigsList& listOut) = 0;

    virtual bool isValidNativeWin(EGLNativeSurfaceType win) = 0;
    virtual bool isValidNativeWin(EGLNativeWindowType win) = 0;
    virtual bool isValidNativePixmap(EGLNativeSurfaceType pix) = 0;

    virtual bool checkWindowPixelFormatMatch(EGLNativeWindowType win,
                                             const EglConfig* config,
                                             unsigned int* width,
                                             unsigned int* height) = 0;

    virtual bool checkPixmapPixelFormatMatch(EGLNativePixmapType pix,
                                             const EglConfig* config,
                                             unsigned int* width,
                                             unsigned int* height) = 0;

    virtual EGLNativeContextType createContext(
            const EglConfig* config, EGLNativeContextType sharedContext) = 0;

    virtual bool destroyContext(EGLNativeContextType context) = 0;

    virtual EGLNativeSurfaceType createPbufferSurface(
            const EglConfig* config, const PbufferInfo* info) = 0;

    virtual bool releasePbuffer(EGLNativeSurfaceType pb) = 0;

    virtual bool makeCurrent(EGLNativeSurfaceType read,
                             EGLNativeSurfaceType draw,
                             EGLNativeContextType context) = 0;

    virtual void swapBuffers(EGLNativeSurfaceType srfc) = 0;

    virtual void swapInterval(EGLNativeSurfaceType win, int interval) = 0;
};

void queryConfigs(EGLNativeInternalDisplayType dpy,int renderable_type,ConfigsList& listOut);
bool releasePbuffer(EGLNativeInternalDisplayType dis,EGLNativeSurfaceType pb);
bool destroyContext(EGLNativeInternalDisplayType dpy,EGLNativeContextType ctx);
bool releaseDisplay(EGLNativeInternalDisplayType dpy);
bool validNativeDisplay(EGLNativeInternalDisplayType dpy);
bool validNativeWin(EGLNativeInternalDisplayType dpy,EGLNativeSurfaceType win);
bool validNativeWin(EGLNativeInternalDisplayType dpy,EGLNativeWindowType win);
bool validNativePixmap(EGLNativeInternalDisplayType dpy,EGLNativeSurfaceType pix);
bool checkWindowPixelFormatMatch(EGLNativeInternalDisplayType dpy,EGLNativeWindowType win,EglConfig* cfg,unsigned int* width,unsigned int* height);
bool checkPixmapPixelFormatMatch(EGLNativeInternalDisplayType dpy,EGLNativePixmapType pix,EglConfig* cfg,unsigned int* width,unsigned int* height);
bool makeCurrent(EGLNativeInternalDisplayType dpy,EGLNativeSurfaceType read, EGLNativeSurfaceType draw,EGLNativeContextType);
void swapBuffers(EGLNativeInternalDisplayType dpy,EGLNativeSurfaceType srfc);
void swapInterval(EGLNativeInternalDisplayType dpy,EGLNativeSurfaceType win,int interval);
void waitNative();

EGLNativeInternalDisplayType getDefaultDisplay();
EGLNativeInternalDisplayType getInternalDisplay(EGLNativeDisplayType dpy);
void deleteDisplay(EGLNativeInternalDisplayType idpy);
EGLNativeSurfaceType createPbufferSurface(EGLNativeInternalDisplayType dpy,EglConfig* cfg, const PbufferInfo* pb);
EGLNativeContextType createContext(EGLNativeInternalDisplayType dpy,EglConfig* cfg,EGLNativeContextType sharedContext);
EGLNativeSurfaceType createWindowSurface(EGLNativeWindowType wnd);
EGLNativeSurfaceType createPixmapSurface(EGLNativePixmapType pix);
void destroySurface(EGLNativeSurfaceType srfc);
#ifdef _WIN32
void initPtrToWglFunctions();
#endif

}  // namespace EglOS

#endif
