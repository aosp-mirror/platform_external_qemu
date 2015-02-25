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
#define MAX_PBUFFER_MIPMAP_LEVEL 1

class SrfcInfo {
public:
    typedef enum {
        WINDOW  = 0,
        PBUFFER = 1,
        PIXMAP
    } SurfaceType;

    SrfcInfo(void* handle, SurfaceType type) :
            m_type(type), m_handle(handle), m_hasMipmap(false) {}

    SurfaceType type() const { return m_type; }

    void* handle() const { return m_handle; }

    bool hasMipmap() const { return m_hasMipmap; }
    void setHasMipmap(bool value) { m_hasMipmap = value; }

private:
    SurfaceType m_type;
    void* m_handle;
    bool m_hasMipmap;
};

namespace {

std::list<EGLNativePixelFormatType> s_nativeFormats;

EglConfig* pixelFormatToConfig(int index,
                               int renderableType,
                               EGLNativePixelFormatType frmt) {
    EGLint  red,green,blue,alpha,depth,stencil;
    EGLint  supportedSurfaces,visualType,visualId;
    EGLint  transparentType,samples;
    EGLint  tRed,tGreen,tBlue;
    EGLint  pMaxWidth,pMaxHeight,pMaxPixels;
    EGLint  level;
    EGLint  window,pbuffer;
    EGLint  doubleBuffer,colorSize;

    getPixelFormatAttrib(frmt,MAC_HAS_DOUBLE_BUFFER,&doubleBuffer);
    if(!doubleBuffer) return NULL; //pixel double buffer

    supportedSurfaces = 0;

    getPixelFormatAttrib(frmt,MAC_DRAW_TO_WINDOW,&window);
    getPixelFormatAttrib(frmt,MAC_DRAW_TO_PBUFFER,&pbuffer);

    if(window)  supportedSurfaces |= EGL_WINDOW_BIT;
    if(pbuffer) supportedSurfaces |= EGL_PBUFFER_BIT;

    if(!supportedSurfaces) return NULL;

    //default values
    visualId                  = 0;
    visualType                = EGL_NONE;
    EGLenum caveat            = EGL_NONE;
    EGLBoolean renderable     = EGL_FALSE;
    pMaxWidth                 = PBUFFER_MAX_WIDTH;
    pMaxHeight                = PBUFFER_MAX_HEIGHT;
    pMaxPixels                = PBUFFER_MAX_PIXELS;
    samples                   = 0;
    level                     = 0;
    tRed = tGreen = tBlue     = 0;

    transparentType = EGL_NONE;

    getPixelFormatAttrib(frmt,MAC_SAMPLES_PER_PIXEL,&samples);
    getPixelFormatAttrib(frmt,MAC_COLOR_SIZE,&colorSize);
    /* All configs can end up having an alpha channel even if none was requested.
     * The default config chooser in GLSurfaceView will therefore not find any
     * matching config. Thus, make sure alpha is zero (or at least signalled as
     * zero to the calling EGL layer) for the configs where it was intended to
     * be zero. */
    if (getPixelFormatDefinitionAlpha(index) == 0)
        alpha = 0;
    else
        getPixelFormatAttrib(frmt,MAC_ALPHA_SIZE,&alpha);
    getPixelFormatAttrib(frmt,MAC_DEPTH_SIZE,&depth);
    getPixelFormatAttrib(frmt,MAC_STENCIL_SIZE,&stencil);

    red = green = blue = (colorSize / 4); //TODO: ask guy if it is OK

    return new EglConfig(red,green,blue,alpha,caveat,(EGLint)index,depth,level,pMaxWidth,pMaxHeight,pMaxPixels,renderable,renderableType,
                         visualId,visualType,samples,stencil,supportedSurfaces,transparentType,tRed,tGreen,tBlue,frmt);
}


void initNativeConfigs(){
    int nConfigs = getNumPixelFormats();
    if(s_nativeFormats.empty()){
        for(int i = 0; i < nConfigs; i++) {
             EGLNativePixelFormatType frmt = getPixelFormat(i);
             if(frmt){
                 s_nativeFormats.push_back(frmt);
             }
        }
    }
}

class MacDisplay : public EglOS::Display {
public:
    explicit MacDisplay(EGLNativeDisplayType dpy) : mDpy(dpy) {}

    virtual bool release() {
        return true;
    }

    virtual void queryConfigs(int renderableType, ConfigsList& listOut) {
        initNativeConfigs();
        int i = 0;
        for (std::list<EGLNativePixelFormatType>::iterator it =
                s_nativeFormats.begin();
            it != s_nativeFormats.end();
            it++) {
            EGLNativePixelFormatType frmt = *it;
            EglConfig* conf = pixelFormatToConfig(i++, renderableType, frmt);
            if(conf) {
                listOut.push_front(conf);
            }
        }
    }

    virtual bool isValidNativeWin(EGLNativeSurfaceType win) {
        if (win->type() != SrfcInfo::WINDOW) {
            return false;
        } else {
            return isValidNativeWin(win->handle());
        }
    }

    virtual bool isValidNativeWin(EGLNativeWindowType win) {
        unsigned int width,height;
        return nsGetWinDims(win, &width, &height);
    }

    virtual bool isValidNativePixmap(EGLNativeSurfaceType pix) {
        // no support for pixmap in mac
        return true;
    }

    virtual bool checkWindowPixelFormatMatch(EGLNativeWindowType win,
                                             const EglConfig* cfg,
                                             unsigned int* width,
                                             unsigned int* height) {
        int r,g,b;
        bool ret = nsGetWinDims(win, width, height);

        cfg->getConfAttrib(EGL_RED_SIZE, &r);
        cfg->getConfAttrib(EGL_GREEN_SIZE, &g);
        cfg->getConfAttrib(EGL_BLUE_SIZE, &b);
        bool match = nsCheckColor(win, r + g + b);

        return ret && match;
    }

    virtual bool checkPixmapPixelFormatMatch(EGLNativePixmapType pix,
                                             const EglConfig* config,
                                             unsigned int* width,
                                             unsigned int* height) {
        return false;
    }

    virtual EGLNativeContextType createContext(
            const EglConfig* config, EGLNativeContextType sharedContext) {
        return nsCreateContext(config->nativeFormat(), sharedContext);
    }

    virtual bool destroyContext(EGLNativeContextType context) {
        nsDestroyContext(context);
        return true;
    }

    virtual EGLNativeSurfaceType createPbufferSurface(
            const EglConfig* cfg, const EglOS::PbufferInfo* info) {
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

        EGLNativeSurfaceType result = new SrfcInfo(
                nsCreatePBuffer(
                        glTexTarget,
                        glTexFormat,
                        maxMipmap,
                        info->width,
                        info->height),
                        SrfcInfo::PBUFFER);

        result->setHasMipmap(info->hasMipmap);
        return result;
    }

    virtual bool releasePbuffer(EGLNativeSurfaceType pb) {
        if (pb) {
            nsDestroyPBuffer(pb->handle());
        }
        return true;
    }

    virtual bool makeCurrent(EGLNativeSurfaceType read,
                             EGLNativeSurfaceType draw,
                             EGLNativeContextType ctx) {
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
        case SrfcInfo::WINDOW:
            nsWindowMakeCurrent(ctx, draw->handle());
            break;
        case SrfcInfo::PBUFFER:
        {
            int mipmapLevel = draw->hasMipmap() ? MAX_PBUFFER_MIPMAP_LEVEL : 0;
            nsPBufferMakeCurrent(ctx, draw->handle(), mipmapLevel);
            break;
        }
        case SrfcInfo::PIXMAP:
        default:
            return false;
        }
        return true;
    }

    virtual void swapBuffers(EGLNativeSurfaceType srfc) {
        nsSwapBuffers();
    }

    virtual void swapInterval(EGLNativeSurfaceType win, int interval) {
        nsSwapInterval(&interval);
    }

    EGLNativeDisplayType dpy() const { return mDpy; }

private:
    EGLNativeDisplayType mDpy;
};

}  // namespace

EGLNativeInternalDisplayType EglOS::getDefaultDisplay() {
    return new MacDisplay(0);
}


EGLNativeInternalDisplayType EglOS::getInternalDisplay(
        EGLNativeDisplayType dpy) {
    return new MacDisplay(dpy);
}

void EglOS::waitNative() {}

EGLNativeSurfaceType EglOS::createWindowSurface(EGLNativeWindowType wnd) {
    return new SrfcInfo(wnd, SrfcInfo::WINDOW);
}

EGLNativeSurfaceType EglOS::createPixmapSurface(EGLNativePixmapType pix) {
    return new SrfcInfo(pix, SrfcInfo::PIXMAP);
}

void EglOS::destroySurface(EGLNativeSurfaceType srfc) {
    delete srfc;
}
