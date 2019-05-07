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
#ifndef _LIBRENDER_COLORBUFFER_H
#define _LIBRENDER_COLORBUFFER_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <GLES3/gl3.h>
#include "android/base/files/Stream.h"
#include "android/snapshot/LazySnapshotObj.h"
#include "emugl/common/smart_ptr.h"
#include "FrameworkFormats.h"
#include "Hwc2.h"
#include "RenderContext.h"

#include <memory>

class TextureDraw;
class TextureResize;
class YUVConverter;

// A class used to model a guest color buffer, and used to implement several
// related things:
//
//  - Every gralloc native buffer with HW read or write requirements will
//    allocate a host ColorBuffer instance. When gralloc_lock() is called,
//    the guest will use ColorBuffer::readPixels() to read the current content
//    of the buffer. When gralloc_unlock() is later called, it will call
//    ColorBuffer::subUpdate() to send the updated pixels.
//
//  - Every guest window EGLSurface is implemented by a host PBuffer
//    (see WindowSurface.h) that can have a ColorBuffer instance attached to
//    it (through WindowSurface::attachColorBuffer()). When such an attachment
//    exists, WindowSurface::flushColorBuffer() will copy the PBuffer's
//    pixel data into the ColorBuffer. The latter can then be displayed
//    in the client's UI sub-window with ColorBuffer::post().
//
//  - Guest EGLImages are implemented as native gralloc buffers too.
//    The guest glEGLImageTargetTexture2DOES() implementations will end up
//    calling ColorBuffer::bindToTexture() to bind the current context's
//    GL_TEXTURE_2D to the buffer. Similarly, the guest versions of
//    glEGLImageTargetRenderbufferStorageOES() will end up calling
//    ColorBuffer::bindToRenderbuffer().
//
// This forces the implementation to use a host EGLImage to implement each
// ColorBuffer.
//
// As an additional twist.

class ColorBuffer :
        public android::snapshot::LazySnapshotObj<ColorBuffer> {
public:
    // Helper interface class used during ColorBuffer operations. This is
    // introduced to remove coupling from the FrameBuffer class implementation.
    class Helper {
    public:
        Helper() = default;
        virtual ~Helper();
        virtual bool setupContext() = 0;
        virtual void teardownContext() = 0;
        virtual TextureDraw* getTextureDraw() const = 0;
        virtual bool isBound() const = 0;
    };

    // Helper class to use a ColorBuffer::Helper context.
    // Usage is pretty simple:
    //
    //     {
    //        RecursiveScopedHelperContext context(m_helper);
    //        if (!context.isOk()) {
    //            return false;   // something bad happened.
    //        }
    //        .... do something ....
    //     }   // automatically calls m_helper->teardownContext();
    //
    class RecursiveScopedHelperContext {
    public:
        RecursiveScopedHelperContext(ColorBuffer::Helper* helper) : mHelper(helper) {
            if (helper->isBound()) return;
            if (!helper->setupContext()) {
                mHelper = NULL;
                return;
            }
            mNeedUnbind = true;
        }

        bool isOk() const { return mHelper != NULL; }

        ~RecursiveScopedHelperContext() { release(); }

        void release() {
            if (mNeedUnbind) {
                mHelper->teardownContext();
                mNeedUnbind = false;
            }
            mHelper = NULL;
        }

    private:
        ColorBuffer::Helper* mHelper;
        bool mNeedUnbind = false;
    };

    // Create a new ColorBuffer instance.
    // |p_display| is the host EGLDisplay handle.
    // |p_width| and |p_height| are the buffer's dimensions in pixels.
    // |p_internalFormat| is the internal OpenGL pixel format to use, valid
    // values
    // are: GL_RGB, GL_RGB565, GL_RGBA, GL_RGB5_A1_OES and GL_RGBA4_OES.
    // Implementation is free to use something else though.
    // |p_frameworkFormat| specifies the original format of the guest
    // color buffer so that we know how to convert to |p_internalFormat|,
    // if necessary (otherwise, p_frameworkFormat ==
    // FRAMEWORK_FORMAT_GL_COMPATIBLE).
    // It is assumed underlying EGL has EGL_KHR_gl_texture_2D_image.
    // Returns NULL on failure.
    // |fastBlitSupported|: whether or not this ColorBuffer can be
    // blitted and posted to swapchain without context switches.
    static ColorBuffer* create(EGLDisplay p_display,
                               int p_width,
                               int p_height,
                               GLenum p_internalFormat,
                               FrameworkFormat p_frameworkFormat,
                               HandleType hndl,
                               Helper* helper,
                               bool fastBlitSupported);

    // Sometimes things happen and we need to reformat the GL texture
    // used. This function replaces the format of the underlying texture
    // with the internalformat specified.
    void reformat(GLint internalformat);

    // Destructor.
    ~ColorBuffer();

    // Return ColorBuffer width and height in pixels
    GLuint getWidth() const { return m_width; }
    GLuint getHeight() const { return m_height; }
    GLint getInternalFormat() const { return m_internalFormat; }

    // Read the ColorBuffer instance's pixel values into host memory.
    void readPixels(int x,
                    int y,
                    int width,
                    int height,
                    GLenum p_format,
                    GLenum p_type,
                    void* pixels);

    // Update the ColorBuffer instance's pixel values from host memory.
    // |p_format / p_type| are the desired OpenGL color buffer format
    // and data type.
    // Otherwise, subUpdate() will explicitly convert |pixels|
    // to be in |p_format|.
    void subUpdate(int x,
                   int y,
                   int width,
                   int height,
                   GLenum p_format,
                   GLenum p_type,
                   void* pixels);

    // Completely replaces contents, assuming that |pixels| is a buffer
    // that is allocated and filled with the same format.
    bool replaceContents(const void* pixels, size_t numBytes);

    // Reads back entire contents, tightly packed rows.
    bool readContents(size_t* numBytes, void* pixels);

    // Draw a ColorBuffer instance, i.e. blit it to the current guest
    // framebuffer object / window surface. This doesn't display anything.
    bool draw();

    // Scale the underlying texture of this ColorBuffer to match viewport size.
    // It returns the texture name after scaling.
    GLuint scale();
    // Post this ColorBuffer to the host native sub-window.
    // |rotation| is the rotation angle in degrees, clockwise in the GL
    // coordinate space.
    bool post(GLuint tex, float rotation, float dx, float dy);
    // Post this ColorBuffer to the host native sub-window and apply
    // the device screen overlay (if there is one).
    // |rotation| is the rotation angle in degrees, clockwise in the GL
    // coordinate space.
    bool postWithOverlay(GLuint tex, float rotation, float dx, float dy);

    // Bind the current context's EGL_TEXTURE_2D texture to this ColorBuffer's
    // EGLImage. This is intended to implement glEGLImageTargetTexture2DOES()
    // for all GLES versions.
    bool bindToTexture();

    // Bind the current context's EGL_RENDERBUFFER_OES render buffer to this
    // ColorBuffer's EGLImage. This is intended to implement
    // glEGLImageTargetRenderbufferStorageOES() for all GLES versions.
    bool bindToRenderbuffer();

    // Copy the content of the current context's read surface to this
    // ColorBuffer. This is used from WindowSurface::flushColorBuffer().
    // Return true on success, false on failure (e.g. no current context).
    bool blitFromCurrentReadBuffer();

    // Read the content of the whole ColorBuffer as 32-bit RGBA pixels.
    // |img| must be a buffer large enough (i.e. width * height * 4).
    void readback(unsigned char* img);
    // readback() but async (to the specified |buffer|)
    void readbackAsync(GLuint buffer);
    // readbackAsync() but in one thread:
    // glReadPixels will be done to buffer1, and then right after,
    // glMapBufferRange -> memcpy(img, <memory of buffer2>)
    void readbackAsync(GLuint buffer1, GLuint buffer2, unsigned char* img);

    void onSave(android::base::Stream* stream);
    static ColorBuffer* onLoad(android::base::Stream* stream,
                               EGLDisplay p_display,
                               Helper* helper,
                               bool fastBlitSupported);

    HandleType getHndl() const;

    bool isFastBlitSupported() const { return m_fastBlitSupported; }
    void postLayer(ComposeLayer* l, int frameWidth, int frameHeight);
    GLuint getTexture();

    bool importMemory(
#ifdef _WIN32
        void* handle,
#else
        int handle,
#endif
        uint64_t size,
        bool dedicated,
        bool linearTiling,
        bool vulkanOnly);
    void setInUse(bool inUse);
    bool isInUse() const { return m_inUse; }

    void setSync(bool debug = false);
    void waitSync(bool debug = false);
    void setDisplay(uint32_t displayId) { m_displayId = displayId; }
    uint32_t getDisplay() { return m_displayId; }

public:
    void restore();

private:
    ColorBuffer(EGLDisplay display, HandleType hndl, Helper* helper);

private:
    GLuint m_tex = 0;
    GLuint m_blitTex = 0;
    EGLImageKHR m_eglImage = nullptr;
    EGLImageKHR m_blitEGLImage = nullptr;
    GLuint m_width = 0;
    GLuint m_height = 0;
    GLuint m_fbo = 0;
    GLenum m_internalFormat = 0;
    GLenum m_sizedInternalFormat = 0;

    // |m_format| and |m_type| are for reformatting purposes only
    // to work around bugs in the guest. No need to snapshot those.
    bool m_needFormatCheck = true;
    GLenum m_format = 0; // TODO: Currently we treat m_internalFormat same as
                         // m_format, but if underlying drivers can take it,
                         // it may be a better idea to distinguish them, with
                         // m_internalFormat as an explicitly sized format; then
                         // guest can specify everything in terms of explicitly
                         // sized internal formats and things will get less
                         // ambiguous.
    GLenum m_type = 0;

    EGLDisplay m_display = nullptr;
    Helper* m_helper = nullptr;
    TextureResize* m_resizer = nullptr;
    FrameworkFormat m_frameworkFormat;
    GLuint m_yuv_conversion_fbo = 0;  // FBO to offscreen-convert YUV to RGB
    std::unique_ptr<YUVConverter> m_yuv_converter;
    HandleType mHndl;

    GLsync m_sync = nullptr;
    bool m_fastBlitSupported = false;

    GLenum m_asyncReadbackType = GL_UNSIGNED_BYTE;
    size_t m_numBytes = 0;

    bool m_importedMemory = false;
    GLuint m_memoryObject = 0;
    bool m_inUse = false;
    bool m_isBuffer = false;
    GLuint m_buf = 0;
    uint32_t m_displayId = 0;
};

typedef emugl::SmartPtr<ColorBuffer> ColorBufferPtr;
#endif
