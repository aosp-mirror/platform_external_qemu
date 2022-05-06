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
#include "ColorBuffer.h"

#include "android/base/Log.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/Tracing.h"

#include "DispatchTables.h"
#include "GLcommon/GLutils.h"
#include "RenderThreadInfo.h"
#include "TextureDraw.h"
#include "TextureResize.h"
#include "YUVConverter.h"

#include "OpenGLESDispatch/EGLDispatch.h"

#include "emugl/common/misc.h"

#include <GLES2/gl2ext.h>

#include <stdio.h>
#include <string.h>

#ifdef NDEBUG
#define DEBUG_CB_FBO 0
#else
#define DEBUG_CB_FBO 1
#endif

namespace {

// Lazily create and bind a framebuffer object to the current host context.
// |fbo| is the address of the framebuffer object name.
// |tex| is the name of a texture that is attached to the framebuffer object
// on creation only. I.e. all rendering operations will target it.
// returns true in case of success, false on failure.
bool bindFbo(GLuint* fbo, GLuint tex) {
    if (*fbo) {
        AEMU_SCOPED_TRACE("bindFbo fast path");
        // fbo already exist - just bind
        s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, *fbo);
        return true;
    }

    AEMU_SCOPED_TRACE("bindFbo slow path");
    s_gles2.glGenFramebuffers(1, fbo);
    s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, *fbo);
    s_gles2.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0_OES,
                                   GL_TEXTURE_2D, tex, 0);
#if DEBUG_CB_FBO
    GLenum status = s_gles2.glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE_OES) {
        ERR("ColorBuffer::bindFbo: FBO not complete: %#x\n", status);
        s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, 0);
        s_gles2.glDeleteFramebuffers(1, fbo);
        *fbo = 0;
        return false;
    }
#endif

    return true;
}

void unbindFbo() {
    s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

}

ColorBuffer::Helper::~Helper() = default;

static GLenum sGetUnsizedColorBufferFormat(GLenum format) {
    switch (format) {
        case GL_R8:
            return GL_RED;
        case GL_RG8:
            return GL_RG;
        case GL_RGB8:
        case GL_RGB565:
        case GL_RGB16F:
            return GL_RGB;
        case GL_RGBA8:
        case GL_RGB5_A1_OES:
        case GL_RGBA4_OES:
        case GL_UNSIGNED_INT_10_10_10_2_OES:
        case GL_RGB10_A2:
        case GL_RGBA16F:
            return GL_RGBA;
        case GL_BGRA8_EXT:
        case GL_BGR10_A2_ANGLEX:
            return GL_BGRA_EXT;
        default: // already unsized
            return format;
    }
}

static bool sGetFormatParameters(GLint* internalFormat,
                                 GLenum* texFormat,
                                 GLenum* pixelType,
                                 int* bytesPerPixel,
                                 GLint* sizedInternalFormat,
                                 bool* isBlob) {
    DCHECK(internalFormat != nullptr);
    *isBlob = false;

    switch (*internalFormat) {
        case GL_RGB:
        case GL_RGB8:
            *texFormat = GL_RGB;
            *pixelType = GL_UNSIGNED_BYTE;
            *bytesPerPixel = 3;
            *sizedInternalFormat = GL_RGB8;
            return true;
        case GL_RGB565_OES:
            *texFormat = GL_RGB;
            *pixelType = GL_UNSIGNED_SHORT_5_6_5;
            *bytesPerPixel = 2;
            *sizedInternalFormat = GL_RGB565;
            return true;
        case GL_RGBA:
        case GL_RGBA8:
        case GL_RGB5_A1_OES:
        case GL_RGBA4_OES:
            *texFormat = GL_RGBA;
            *pixelType = GL_UNSIGNED_BYTE;
            *bytesPerPixel = 4;
            *sizedInternalFormat = GL_RGBA8;
            return true;
        case GL_UNSIGNED_INT_10_10_10_2_OES:
            *texFormat = GL_RGBA;
            *pixelType = GL_UNSIGNED_SHORT;
            *bytesPerPixel = 4;
            *sizedInternalFormat = GL_UNSIGNED_INT_10_10_10_2_OES;
            return true;
        case GL_RGB10_A2:
            *texFormat = GL_RGBA;
            *pixelType = GL_UNSIGNED_INT_2_10_10_10_REV;
            *bytesPerPixel = 4;
            *sizedInternalFormat = GL_RGB10_A2;
            return true;
        case GL_RGB16F:
            *texFormat = GL_RGB;
            *pixelType = GL_HALF_FLOAT;
            *bytesPerPixel = 6;
            *sizedInternalFormat = GL_RGB16F;
            return true;
        case GL_RGBA16F:
            *texFormat = GL_RGBA;
            *pixelType = GL_HALF_FLOAT;
            *bytesPerPixel = 8;
            *sizedInternalFormat = GL_RGBA16F;
            return true;
        case GL_LUMINANCE:
            *texFormat = GL_LUMINANCE;
            *pixelType = GL_UNSIGNED_BYTE;
            *bytesPerPixel = 1;
            *sizedInternalFormat = GL_R8;
            *isBlob = true;
            return true;
        case GL_BGRA_EXT:
            *texFormat = GL_BGRA_EXT;
            *pixelType = GL_UNSIGNED_BYTE;
            *bytesPerPixel = 4;
            *sizedInternalFormat = GL_BGRA8_EXT;
            return true;
        case GL_BGR10_A2_ANGLEX:
            *texFormat = GL_RGBA;
            *pixelType = GL_UNSIGNED_INT_2_10_10_10_REV;
            *bytesPerPixel = 4;
            *internalFormat = GL_RGB10_A2_EXT;
            // GL_BGR10_A2_ANGLEX is actually not a valid GL format. We should
            // replace it with a normal GL internal format instead.
            *sizedInternalFormat = GL_BGR10_A2_ANGLEX;
            return true;
        case GL_R8:
        case GL_RED:
            *texFormat = GL_RED;
            *pixelType = GL_UNSIGNED_BYTE;
            *bytesPerPixel = 1;
            *sizedInternalFormat = GL_R8;
            return true;
        case GL_RG8:
        case GL_RG:
            *texFormat = GL_RG;
            *pixelType = GL_UNSIGNED_BYTE;
            *bytesPerPixel = 2;
            *sizedInternalFormat = GL_RG8;
            return true;
        default:
            fprintf(stderr, "%s: Unknown format 0x%x\n", __func__,
                    *internalFormat);
            return false;
    }
}

// static
ColorBuffer* ColorBuffer::create(EGLDisplay p_display,
                                 int p_width,
                                 int p_height,
                                 GLint p_internalFormat,
                                 FrameworkFormat p_frameworkFormat,
                                 HandleType hndl,
                                 Helper* helper,
                                 bool fastBlitSupported) {
    AEMU_SCOPED_TRACE("ColorBuffer::create");
    GLenum texFormat = 0;
    GLenum pixelType = GL_UNSIGNED_BYTE;
    int bytesPerPixel = 4;
    GLint p_sizedInternalFormat = GL_RGBA8;
    bool isBlob = false;;

    if (!sGetFormatParameters(&p_internalFormat, &texFormat, &pixelType,
                              &bytesPerPixel, &p_sizedInternalFormat,
                              &isBlob)) {
        fprintf(stderr, "ColorBuffer::create invalid format 0x%x\n",
                p_internalFormat);
        return NULL;
    }

    android::base::beginTrace("initialImageAlloc");
    const unsigned long bufsize = ((unsigned long)bytesPerPixel) * p_width
            * p_height;
    // android::base::ScopedCPtr<char> initialImage(
    //             static_cast<char*>(::malloc(bufsize)));
    // if (!initialImage) {
    //     fprintf(stderr,
    //             "error: failed to allocate initial memory for ColorBuffer "
    //             "of size %dx%dx%d (%lu KB)\n",
    //             p_width, p_height, bytesPerPixel * 8, bufsize / 1024);
    //     return nullptr;
    // }
    // memset(initialImage.get(), 0x0, bufsize);
    android::base::endTrace();

    android::base::beginTrace("BindContext");
    RecursiveScopedHelperContext context(helper);
    android::base::endTrace();
    if (!context.isOk()) {
        return NULL;
    }

    android::base::beginTrace("colorBuffer ctor");
    std::unique_ptr<ColorBuffer> cb(new ColorBuffer(p_display, hndl, helper));
    android::base::endTrace();

    GLint prevUnpackAlignment;
    s_gles2.glGetIntegerv(GL_UNPACK_ALIGNMENT, &prevUnpackAlignment);
    s_gles2.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    s_gles2.glGenTextures(1, &cb->m_tex);
    s_gles2.glBindTexture(GL_TEXTURE_2D, cb->m_tex);

    android::base::beginTrace("initialImage upload");
    GLenum err = s_gles2.glGetError();
    if (err) {
        ERR("ColorBuffer::create get gl error 0x%x before creating textures", err);
    }
    s_gles2.glTexImage2D(GL_TEXTURE_2D, 0, p_internalFormat, p_width, p_height,
                         0, texFormat, pixelType, 0);
    android::base::endTrace();
    if (s_gles2.glGetError()) {
        return nullptr;
    }
    android::base::beginTrace("initialImage dtor");
    // initialImage.reset();
    android::base::endTrace();

    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // Swizzle B/R channel for BGR10_A2 images.
    if (p_sizedInternalFormat == GL_BGR10_A2_ANGLEX) {
        s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
        s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
        cb->m_BRSwizzle = true;
    }

    //
    // create another texture for that colorbuffer for blit
    //
    s_gles2.glGenTextures(1, &cb->m_blitTex);
    s_gles2.glBindTexture(GL_TEXTURE_2D, cb->m_blitTex);
    android::base::beginTrace("secondary blit upload");
    s_gles2.glTexImage2D(GL_TEXTURE_2D, 0, p_internalFormat, p_width, p_height,
                         0, texFormat, pixelType, NULL);
    android::base::endTrace();

    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // Swizzle B/R channel for BGR10_A2 images.
    if (p_sizedInternalFormat == GL_BGR10_A2_ANGLEX) {
        s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
        s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
        cb->m_BRSwizzle = true;
    }

    cb->m_width = p_width;
    cb->m_height = p_height;
    cb->m_internalFormat = p_internalFormat;
    cb->m_sizedInternalFormat = p_sizedInternalFormat;
    cb->m_format = texFormat;
    cb->m_type = pixelType;

    android::base::beginTrace("eglcreateimage");
    cb->m_eglImage = s_egl.eglCreateImageKHR(
            p_display, s_egl.eglGetCurrentContext(), EGL_GL_TEXTURE_2D_KHR,
            (EGLClientBuffer)SafePointerFromUInt(cb->m_tex), NULL);

    cb->m_blitEGLImage = s_egl.eglCreateImageKHR(
            p_display, s_egl.eglGetCurrentContext(), EGL_GL_TEXTURE_2D_KHR,
            (EGLClientBuffer)SafePointerFromUInt(cb->m_blitTex), NULL);
    android::base::endTrace();

    android::base::beginTrace("TextureResize create");
    cb->m_resizer = new TextureResize(p_width, p_height);
    android::base::endTrace();

    cb->m_frameworkFormat = p_frameworkFormat;
    switch (cb->m_frameworkFormat) {
        case FRAMEWORK_FORMAT_GL_COMPATIBLE:
            break;
        default: // Any YUV format
            cb->m_yuv_converter.reset(
                    new YUVConverter(p_width, p_height, cb->m_frameworkFormat));
            break;
    }

    cb->m_fastBlitSupported = fastBlitSupported;

    // desktop GL only: use GL_UNSIGNED_INT_8_8_8_8_REV for faster readback.
    if (emugl::getRenderer() == SELECTED_RENDERER_HOST) {
#define GL_UNSIGNED_INT_8_8_8_8           0x8035
#define GL_UNSIGNED_INT_8_8_8_8_REV       0x8367
        cb->m_asyncReadbackType = GL_UNSIGNED_INT_8_8_8_8_REV;
    }

    cb->m_numBytes = (size_t)bufsize;

    s_gles2.glPixelStorei(GL_UNPACK_ALIGNMENT, prevUnpackAlignment);

    android::base::beginTrace("glFinish");
    // s_gles2.glFinish();
    android::base::endTrace();
    return cb.release();
}

ColorBuffer::ColorBuffer(EGLDisplay display, HandleType hndl, Helper* helper)
    : m_display(display), m_helper(helper), mHndl(hndl) {}

ColorBuffer::~ColorBuffer() {
    RecursiveScopedHelperContext context(m_helper);

    if (m_blitEGLImage) {
        s_egl.eglDestroyImageKHR(m_display, m_blitEGLImage);
    }
    if (m_eglImage) {
        s_egl.eglDestroyImageKHR(m_display, m_eglImage);
    }

    if (m_fbo) {
        s_gles2.glDeleteFramebuffers(1, &m_fbo);
    }

    if (m_yuv_conversion_fbo) {
        s_gles2.glDeleteFramebuffers(1, &m_yuv_conversion_fbo);
    }

    if (m_scaleRotationFbo) {
        s_gles2.glDeleteFramebuffers(1, &m_scaleRotationFbo);
    }

    m_yuv_converter.reset();

    GLuint tex[2] = {m_tex, m_blitTex};
    s_gles2.glDeleteTextures(2, tex);

    if (m_memoryObject) {
        s_gles2.glDeleteMemoryObjectsEXT(1, &m_memoryObject);
    }

    delete m_resizer;
}

void ColorBuffer::readPixels(int x,
                             int y,
                             int width,
                             int height,
                             GLenum p_format,
                             GLenum p_type,
                             void* pixels) {
    RecursiveScopedHelperContext context(m_helper);
    if (!context.isOk()) {
        return;
    }
    p_format = sGetUnsizedColorBufferFormat(p_format);
    touch();

    if (bindFbo(&m_fbo, m_tex)) {
        GLint prevAlignment = 0;
        AEMU_SCOPED_TRACE("readPixels api calls inner");
        if (m_readPixelsDirty) {
            AEMU_SCOPED_TRACE("readPixelsDirty!");
        } else {
            AEMU_SCOPED_TRACE("readPixelsNotDirty!");
        }
        s_gles2.glGetIntegerv(GL_PACK_ALIGNMENT, &prevAlignment);
        s_gles2.glPixelStorei(GL_PACK_ALIGNMENT, 1);
        s_gles2.glReadPixels(x, y, width, height, p_format, p_type, pixels);
        s_gles2.glPixelStorei(GL_PACK_ALIGNMENT, prevAlignment);
        unbindFbo();
    }
    m_readPixelsDirty = false;
}

void ColorBuffer::readPixelsScaled(int width,
                                   int height,
                                   GLenum p_format,
                                   GLenum p_type,
                                   SkinRotation rotation,
                                   void* pixels,
                                   SkinRect rect) {
    RecursiveScopedHelperContext context(m_helper);
    if (!context.isOk()) {
        return;
    }
    bool useSnipping = rect.size.w != 0 && rect.size.h != 0;
    // Boundary check
    if (useSnipping &&
        (rect.pos.x < 0 || rect.pos.y < 0 || rect.pos.x + rect.size.w > width ||
         rect.pos.y + rect.size.h > height)) {
        LOG(ERROR) << "readPixelsScaled failed. Out-of-bound rectangle: ("
                   << rect.pos.x << ", " << rect.pos.y << ") [" << rect.size.w
                   << " x " << rect.size.h << "] "
                   << "with screen "
                   << "[" << width << " x " << height << "] ";
        return;
    }
    p_format = sGetUnsizedColorBufferFormat(p_format);
    touch();
    GLuint tex = m_resizer->update(m_tex, width, height, rotation);
    if (bindFbo(&m_scaleRotationFbo, tex)) {
        GLint prevAlignment = 0;
        s_gles2.glGetIntegerv(GL_PACK_ALIGNMENT, &prevAlignment);
        s_gles2.glPixelStorei(GL_PACK_ALIGNMENT, 1);
        if (useSnipping) {
            s_gles2.glReadPixels(rect.pos.x, rect.pos.y, rect.size.w,
                                 rect.size.h, p_format, p_type, pixels);
        } else {
            s_gles2.glReadPixels(0, 0, width, height, p_format, p_type, pixels);
        }
        s_gles2.glPixelStorei(GL_PACK_ALIGNMENT, prevAlignment);
        unbindFbo();
    }
}

void ColorBuffer::readPixelsYUVCached(int x,
                                      int y,
                                      int width,
                                      int height,
                                      void* pixels,
                                      uint32_t pixels_size) {
    RecursiveScopedHelperContext context(m_helper);
    if (!context.isOk()) {
        return;
    }

    touch();

#if DEBUG_CB_FBO
    fprintf(stderr, "%s %d request width %d height %d\n", __func__, __LINE__,
            width, height);
    memset(pixels, 0x00, pixels_size);
    assert(m_yuv_converter.get());
#endif


    m_yuv_converter->readPixels((uint8_t*)pixels, pixels_size);

    return;
}

void ColorBuffer::reformat(GLint internalformat, GLenum type) {
    GLenum texFormat = internalformat;
    GLenum pixelType = GL_UNSIGNED_BYTE;
    GLint sizedInternalFormat = GL_RGBA8;
    int bpp = 4;
    bool isBlob = false;
    if (!sGetFormatParameters(&internalformat, &texFormat, &pixelType, &bpp,
                              &sizedInternalFormat, &isBlob)) {
        dwarning("%s: reformat failed. internal format: 0x%x",
                __func__, internalformat);
    }

    // BUG: 143607546
    //
    // During reformatting, sGetFormatParameters can be too
    // opinionated and override the guest's intended choice for the
    // pixel type.  If the guest wanted GL_UNSIGNED_SHORT_5_6_5 as
    // the pixel type, and the incoming internal format is not
    // explicitly sized, sGetFormatParameters will pick a default of
    // GL_UNSIGNED BYTE, which goes against guest expectations.
    //
    // This happens only on older API levels where gralloc.cpp in
    // goldfish-opengl communicated HAL_PIXEL_FORMAT_RGB_565 as GL
    // format GL_RGB, pixel type GL_UNSIGNED_SHORT_5_6_5.  Newer
    // system images communicate HAL_PIXEL_FORMAT_RGB_565 as GL
    // format GL_RGB565, which allows sGetFormatParameters to work
    // correctly.
    if (pixelType != type) {
        pixelType = type;
    }

    s_gles2.glBindTexture(GL_TEXTURE_2D, m_tex);
    s_gles2.glTexImage2D(GL_TEXTURE_2D, 0, internalformat, m_width, m_height,
                         0, texFormat, pixelType, nullptr);

    s_gles2.glBindTexture(GL_TEXTURE_2D, m_blitTex);
    s_gles2.glTexImage2D(GL_TEXTURE_2D, 0, internalformat, m_width, m_height,
                         0, texFormat, pixelType, nullptr);

    // EGL images need to be recreated because the EGL_KHR_image_base spec
    // states that respecifying an image (i.e. glTexImage2D) will generally
    // result in orphaning of the EGL image.
    s_egl.eglDestroyImageKHR(m_display, m_eglImage);
    m_eglImage = s_egl.eglCreateImageKHR(
            m_display, s_egl.eglGetCurrentContext(), EGL_GL_TEXTURE_2D_KHR,
            (EGLClientBuffer)SafePointerFromUInt(m_tex), NULL);

    s_egl.eglDestroyImageKHR(m_display, m_blitEGLImage);
    m_blitEGLImage = s_egl.eglCreateImageKHR(
            m_display, s_egl.eglGetCurrentContext(), EGL_GL_TEXTURE_2D_KHR,
            (EGLClientBuffer)SafePointerFromUInt(m_blitTex), NULL);

    s_gles2.glBindTexture(GL_TEXTURE_2D, 0);

    m_internalFormat = internalformat;
    m_format = texFormat;
    m_type = pixelType;
    m_sizedInternalFormat = sizedInternalFormat;

    m_numBytes = bpp * m_width * m_height;
}

void ColorBuffer::swapYUVTextures(uint32_t type, uint32_t* textures) {
    if (type == FRAMEWORK_FORMAT_NV12) {
        m_yuv_converter->swapTextures(type, textures);
    } else {
        fprintf(stderr,
                "%s: ERROR: format other than NV12 is not supported: 0x%x\n",
                __func__, type);
    }
}

void ColorBuffer::subUpdate(int x,
                            int y,
                            int width,
                            int height,
                            GLenum p_format,
                            GLenum p_type,
                            void* pixels) {
    subUpdateFromFrameworkFormat(x, y, width, height, m_frameworkFormat, p_format, p_type, pixels);
}

void ColorBuffer::subUpdateFromFrameworkFormat(int x,
                                               int y,
                                               int width,
                                               int height,
                                               FrameworkFormat fwkFormat,
                                               GLenum p_format,
                                               GLenum p_type,
                                               void* pixels) {
    const GLenum p_unsizedFormat = sGetUnsizedColorBufferFormat(p_format);
    RecursiveScopedHelperContext context(m_helper);

    if (!context.isOk()) {
        return;
    }

    touch();

    if (m_needFormatCheck) {
        if (p_type != m_type || p_format != m_format) {
            reformat((GLint)p_format, p_type);
        }
        m_needFormatCheck = false;
    }

    if (m_frameworkFormat != FRAMEWORK_FORMAT_GL_COMPATIBLE ||
        fwkFormat != m_frameworkFormat) {
        assert(m_yuv_converter.get());

        // This FBO will convert the YUV frame to RGB
        // and render it to |m_tex|.
        bindFbo(&m_yuv_conversion_fbo, m_tex);
        m_yuv_converter->drawConvertFromFormat(fwkFormat, x, y, width, height, (char*)pixels);
        unbindFbo();

        // |m_tex| still needs to be bound afterwards
        s_gles2.glBindTexture(GL_TEXTURE_2D, m_tex);

    } else {
        s_gles2.glBindTexture(GL_TEXTURE_2D, m_tex);
        s_gles2.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        s_gles2.glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, p_unsizedFormat,
                                p_type, pixels);
    }

    if (m_fastBlitSupported) {
        s_gles2.glFlush();
        m_sync = (GLsync)s_egl.eglSetImageFenceANDROID(m_display, m_eglImage);
    }
}

bool ColorBuffer::replaceContents(const void* newContents, size_t numBytes) {
    RecursiveScopedHelperContext context(m_helper);
    if (!context.isOk()) {
        fprintf(stderr, "%s: Failed: Could not get current context\n", __func__);
        return false;
    }

    if (m_numBytes != numBytes) {
        fprintf(stderr,
            "%s: Error: Tried to replace contents of ColorBuffer with "
            "%zu bytes (expected %zu; GL format info: 0x%x 0x%x 0x%x); ",
            __func__,
            numBytes,
            m_numBytes,
            m_internalFormat,
            m_format,
            m_type);
        return false;
    }

    touch();

    s_gles2.glBindTexture(GL_TEXTURE_2D, m_tex);
    s_gles2.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    s_gles2.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_width, m_height, m_format,
                            m_type, newContents);

    if (m_fastBlitSupported) {
        s_gles2.glFlush();
        m_sync = (GLsync)s_egl.eglSetImageFenceANDROID(m_display, m_eglImage);
    }

    m_readPixelsDirty = true;
    return true;
}

bool ColorBuffer::readContents(size_t* numBytes, void* pixels) {
    if (m_yuv_converter) {
        *numBytes = m_yuv_converter->getDataSize();
        if (pixels) {
            readPixelsYUVCached(0, 0, 0, 0, pixels, *numBytes);
        }
        return true;
    } else {
        RecursiveScopedHelperContext context(m_helper);
        *numBytes = m_numBytes;

        if (!pixels) return true;

        readPixels(0, 0, m_width, m_height, m_format, m_type, pixels);

        return true;
    }
}

bool ColorBuffer::blitFromCurrentReadBuffer() {
    RenderThreadInfo* tInfo = RenderThreadInfo::get();
    if (!tInfo->currContext.get()) {
        // no Current context
        return false;
    }

    touch();

    if (m_fastBlitSupported) {
        s_egl.eglBlitFromCurrentReadBufferANDROID(m_display, m_eglImage);
        m_sync = (GLsync)s_egl.eglSetImageFenceANDROID(m_display, m_eglImage);
    } else {
        // Copy the content of the current read surface into m_blitEGLImage.
        // This is done by creating a temporary texture, bind it to the EGLImage
        // then call glCopyTexSubImage2D().
        GLuint tmpTex;
        GLint currTexBind;
        if (tInfo->currContext->clientVersion() > GLESApi_CM) {
            s_gles2.glGetIntegerv(GL_TEXTURE_BINDING_2D, &currTexBind);
            s_gles2.glGenTextures(1, &tmpTex);
            s_gles2.glBindTexture(GL_TEXTURE_2D, tmpTex);
            s_gles2.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_blitEGLImage);

            const bool isGles3 = tInfo->currContext->clientVersion() > GLESApi_2;

            GLint prev_read_fbo = 0;
            if (isGles3) {
                // Make sure that we unbind any existing GL_READ_FRAMEBUFFER
                // before calling glCopyTexSubImage2D, otherwise we may blit
                // from the guest's current read framebuffer instead of the EGL
                // read buffer.
                s_gles2.glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prev_read_fbo);
                if (prev_read_fbo != 0) {
                    s_gles2.glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
                }
            } else {
                // On GLES 2, there are not separate read/draw framebuffers,
                // only GL_FRAMEBUFFER.  Per the EGL 1.4 spec section 3.9.3,
                // the draw surface must be bound to the calling thread's
                // current context, so GL_FRAMEBUFFER should be 0.  However, the
                // error case is not strongly defined and generating a new error
                // may break existing apps.
                //
                // Instead of the obviously wrong behavior of posting whatever
                // GL_FRAMEBUFFER is currently bound to, fix up the
                // GL_FRAMEBUFFER if it is non-zero.
                s_gles2.glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_read_fbo);
                if (prev_read_fbo != 0) {
                    s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, 0);
                }
            }

            // If the read buffer is multisampled, we need to resolve.
            GLint samples;
            s_gles2.glGetIntegerv(GL_SAMPLE_BUFFERS, &samples);
            if (isGles3 && samples > 0) {
                s_gles2.glBindTexture(GL_TEXTURE_2D, 0);

                GLuint resolve_fbo;
                GLint prev_draw_fbo;
                s_gles2.glGenFramebuffers(1, &resolve_fbo);
                s_gles2.glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prev_draw_fbo);

                s_gles2.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolve_fbo);
                s_gles2.glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
                        GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                        tmpTex, 0);
                s_gles2.glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width,
                        m_height, GL_COLOR_BUFFER_BIT,
                        GL_NEAREST);
                s_gles2.glBindFramebuffer(GL_DRAW_FRAMEBUFFER,
                        (GLuint)prev_draw_fbo);

                s_gles2.glDeleteFramebuffers(1, &resolve_fbo);
                s_gles2.glBindTexture(GL_TEXTURE_2D, tmpTex);
            } else {
                // If the buffer is not multisampled, perform a normal texture copy.
                s_gles2.glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, m_width,
                        m_height);
            }

            if (prev_read_fbo != 0) {
                if (isGles3) {
                    s_gles2.glBindFramebuffer(GL_READ_FRAMEBUFFER,
                                              (GLuint)prev_read_fbo);
                } else {
                    s_gles2.glBindFramebuffer(GL_FRAMEBUFFER,
                                              (GLuint)prev_read_fbo);
                }
            }

            s_gles2.glDeleteTextures(1, &tmpTex);
            s_gles2.glBindTexture(GL_TEXTURE_2D, currTexBind);

            // clear GL errors, because its possible that the fbo format does not
            // match
            // the format of the read buffer, in the case of OpenGL ES 3.1 and
            // integer
            // RGBA formats.
            s_gles2.glGetError();
            // This is currently for dEQP purposes only; if we actually want these
            // integer FBO formats to actually serve to display something for human
            // consumption,
            // we need to change the egl image to be of the same format,
            // or we get some really psychedelic patterns.
        } else {
            // Like in the GLES 2 path above, correct the case where
            // GL_FRAMEBUFFER_OES is not bound to zero so that we don't blit
            // from arbitrary framebuffers.
            // Use GLES 2 because it internally has the same value as the GLES 1
            // API and it doesn't require GL_OES_framebuffer_object.
            GLint prev_fbo = 0;
            s_gles2.glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
            if (prev_fbo != 0) {
                s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }

            s_gles1.glGetIntegerv(GL_TEXTURE_BINDING_2D, &currTexBind);
            s_gles1.glGenTextures(1, &tmpTex);
            s_gles1.glBindTexture(GL_TEXTURE_2D, tmpTex);
            s_gles1.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_blitEGLImage);
            s_gles1.glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, m_width,
                    m_height);
            s_gles1.glDeleteTextures(1, &tmpTex);
            s_gles1.glBindTexture(GL_TEXTURE_2D, currTexBind);

            if (prev_fbo != 0) {
                s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)prev_fbo);
            }
        }

        RecursiveScopedHelperContext context(m_helper);
        if (!context.isOk()) {
            return false;
        }

        if (!bindFbo(&m_fbo, m_tex)) {
            return false;
        }

        // Save current viewport and match it to the current colorbuffer size.
        GLint vport[4] = {
            0,
        };
        s_gles2.glGetIntegerv(GL_VIEWPORT, vport);
        s_gles2.glViewport(0, 0, m_width, m_height);

        // render m_blitTex
        m_helper->getTextureDraw()->draw(m_blitTex, 0., 0, 0);

        // Restore previous viewport.
        s_gles2.glViewport(vport[0], vport[1], vport[2], vport[3]);
        unbindFbo();
    }

    return true;
}

bool ColorBuffer::bindToTexture() {
    if (!m_eglImage) {
        return false;
    }

    RenderThreadInfo* tInfo = RenderThreadInfo::get();
    if (!tInfo->currContext.get()) {
        return false;
    }
    touch();

    if (tInfo->currContext->clientVersion() > GLESApi_CM) {
        s_gles2.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_eglImage);
    } else {
        s_gles1.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_eglImage);
    }
    m_readPixelsDirty = true;
    return true;
}

bool ColorBuffer::bindToTexture2() {
    if (!m_eglImage) {
        return false;
    }

    s_gles2.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_eglImage);
    m_readPixelsDirty = true;
    return true;
}

bool ColorBuffer::bindToRenderbuffer() {
    if (!m_eglImage) {
        return false;
    }
    RenderThreadInfo* tInfo = RenderThreadInfo::get();
    if (!tInfo->currContext.get()) {
        return false;
    }
    touch();
    if (tInfo->currContext->clientVersion() > GLESApi_CM) {
        s_gles2.glEGLImageTargetRenderbufferStorageOES(GL_RENDERBUFFER_OES,
                                                       m_eglImage);
    } else {
        s_gles1.glEGLImageTargetRenderbufferStorageOES(GL_RENDERBUFFER_OES,
                                                       m_eglImage);
    }
    m_readPixelsDirty = true;
    return true;
}

GLuint ColorBuffer::scale() {
    return m_resizer->update(m_tex);
}

void ColorBuffer::setSync(bool debug) {
    m_sync = (GLsync)s_egl.eglSetImageFenceANDROID(m_display, m_eglImage);
    if (debug) fprintf(stderr, "%s: %u to %p\n", __func__, getHndl(), m_sync);
}

void ColorBuffer::waitSync(bool debug) {
    if (debug) fprintf(stderr, "%s: %u sync %p\n", __func__, getHndl(), m_sync);
    if (m_sync) {
        s_egl.eglWaitImageFenceANDROID(m_display, m_sync);
    }
}

bool ColorBuffer::post(GLuint tex, float rotation, float dx, float dy) {
    // NOTE: Do not call m_helper->setupContext() here!
    waitSync();
    return m_helper->getTextureDraw()->draw(tex, rotation, dx, dy);
}

bool ColorBuffer::postWithOverlay(GLuint tex, float rotation, float dx, float dy) {
    // NOTE: Do not call m_helper->setupContext() here!
    waitSync();
    return m_helper->getTextureDraw()->drawWithOverlay(tex, rotation, dx, dy);
}

void ColorBuffer::readback(unsigned char* img, bool readbackBgra) {
    RecursiveScopedHelperContext context(m_helper);
    if (!context.isOk()) {
        return;
    }
    touch();
    waitSync();

    if (bindFbo(&m_fbo, m_tex)) {
        // Flip the readback format if RED/BLUE components are swizzled.
        bool shouldReadbackBgra = m_BRSwizzle ? !readbackBgra : readbackBgra;
        GLenum format = shouldReadbackBgra ? GL_BGRA_EXT : GL_RGBA;

        s_gles2.glReadPixels(0, 0, m_width, m_height, format, GL_UNSIGNED_BYTE, img);
        unbindFbo();
    }
}

void ColorBuffer::readbackAsync(GLuint buffer, bool readbackBgra) {
    RecursiveScopedHelperContext context(m_helper);
    if (!context.isOk()) {
        return;
    }
    touch();
    waitSync();

    if (bindFbo(&m_fbo, m_tex)) {
        s_gles2.glBindBuffer(GL_PIXEL_PACK_BUFFER, buffer);
        bool shouldReadbackBgra = m_BRSwizzle ? !readbackBgra : readbackBgra;
        GLenum format = shouldReadbackBgra ? GL_BGRA_EXT : GL_RGBA;
        s_gles2.glReadPixels(0, 0, m_width, m_height, format, m_asyncReadbackType, 0);
        s_gles2.glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        unbindFbo();
    }
}

HandleType ColorBuffer::getHndl() const {
    return mHndl;
}

void ColorBuffer::onSave(android::base::Stream* stream) {
    stream->putBe32(getHndl());
    stream->putBe32(static_cast<uint32_t>(m_width));
    stream->putBe32(static_cast<uint32_t>(m_height));
    stream->putBe32(static_cast<uint32_t>(m_internalFormat));
    stream->putBe32(static_cast<uint32_t>(m_frameworkFormat));
    // for debug
    assert(m_eglImage && m_blitEGLImage);
    stream->putBe32(reinterpret_cast<uintptr_t>(m_eglImage));
    stream->putBe32(reinterpret_cast<uintptr_t>(m_blitEGLImage));
    stream->putBe32(m_needFormatCheck);
}

ColorBuffer* ColorBuffer::onLoad(android::base::Stream* stream,
                                 EGLDisplay p_display,
                                 Helper* helper,
                                 bool fastBlitSupported) {
    HandleType hndl = static_cast<HandleType>(stream->getBe32());
    GLuint width = static_cast<GLuint>(stream->getBe32());
    GLuint height = static_cast<GLuint>(stream->getBe32());
    GLenum internalFormat = static_cast<GLenum>(stream->getBe32());
    FrameworkFormat frameworkFormat =
            static_cast<FrameworkFormat>(stream->getBe32());
    EGLImageKHR eglImage = reinterpret_cast<EGLImageKHR>(stream->getBe32());
    EGLImageKHR blitEGLImage = reinterpret_cast<EGLImageKHR>(stream->getBe32());
    uint32_t needFormatCheck = stream->getBe32();

    if (!eglImage) {
        return create(p_display, width, height, internalFormat, frameworkFormat,
                      hndl, helper, fastBlitSupported);
    }
    ColorBuffer* cb = new ColorBuffer(p_display, hndl, helper);
    cb->mNeedRestore = true;
    cb->m_eglImage = eglImage;
    cb->m_blitEGLImage = blitEGLImage;
    assert(eglImage && blitEGLImage);
    cb->m_width = width;
    cb->m_height = height;
    cb->m_internalFormat = internalFormat;
    cb->m_frameworkFormat = frameworkFormat;
    cb->m_fastBlitSupported = fastBlitSupported;
    cb->m_needFormatCheck = needFormatCheck;
    return cb;
}

void ColorBuffer::restore() {
    RecursiveScopedHelperContext context(m_helper);
    s_gles2.glGenTextures(1, &m_tex);
    s_gles2.glBindTexture(GL_TEXTURE_2D, m_tex);
    s_gles2.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_eglImage);

    s_gles2.glGenTextures(1, &m_blitTex);
    s_gles2.glBindTexture(GL_TEXTURE_2D, m_blitTex);
    s_gles2.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_blitEGLImage);

    m_resizer = new TextureResize(m_width, m_height);
    switch (m_frameworkFormat) {
        case FRAMEWORK_FORMAT_GL_COMPATIBLE:
            break;
        case FRAMEWORK_FORMAT_YV12:
        case FRAMEWORK_FORMAT_YUV_420_888:
            m_yuv_converter.reset(
                    new YUVConverter(m_width, m_height, m_frameworkFormat));
            break;
        default:
            break;
    }
}


GLuint ColorBuffer::getTexture() {
    touch();
    return m_tex;
}

void ColorBuffer::postLayer(ComposeLayer* l, int frameWidth, int frameHeight) {
    if (m_inUse) fprintf(stderr, "%s: cb in use\n", __func__);
    waitSync();
    m_helper->getTextureDraw()->drawLayer(l, frameWidth, frameHeight, m_width, m_height, m_tex);
}

bool ColorBuffer::importMemory(
#ifdef _WIN32
        void* handle,
#else
        int handle,
#endif
        uint64_t size,
        bool dedicated,
        bool linearTiling,
        bool vulkanOnly) {
    RecursiveScopedHelperContext context(m_helper);
    s_gles2.glCreateMemoryObjectsEXT(1, &m_memoryObject);
    if (dedicated) {
        static const GLint DEDICATED_FLAG = GL_TRUE;
        s_gles2.glMemoryObjectParameterivEXT(m_memoryObject,
                                             GL_DEDICATED_MEMORY_OBJECT_EXT,
                                             &DEDICATED_FLAG);
    }

#ifdef _WIN32
    s_gles2.glImportMemoryWin32HandleEXT(m_memoryObject, size, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, handle);
#else
    s_gles2.glImportMemoryFdEXT(m_memoryObject, size, GL_HANDLE_TYPE_OPAQUE_FD_EXT, handle);
#endif

    GLuint glTiling = linearTiling ? GL_LINEAR_TILING_EXT : GL_OPTIMAL_TILING_EXT;

    std::vector<uint8_t> prevContents;

    if (!vulkanOnly) {
        size_t bytes;
        readContents(&bytes, nullptr);
        prevContents.resize(bytes, 0);
        readContents(&bytes, prevContents.data());
    }

    s_gles2.glDeleteTextures(1, &m_tex);
    s_gles2.glGenTextures(1, &m_tex);
    s_gles2.glBindTexture(GL_TEXTURE_2D, m_tex);

    // HOST needed because we do not expose this to guest
    s_gles2.glTexParameteriHOST(GL_TEXTURE_2D, GL_TEXTURE_TILING_EXT, glTiling);

    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (m_sizedInternalFormat == GL_BGRA8_EXT ||
        m_sizedInternalFormat == GL_BGR10_A2_ANGLEX) {
        GLint internalFormat = m_sizedInternalFormat == GL_BGRA8_EXT
                                       ? GL_RGBA8
                                       : GL_RGB10_A2_EXT;
        s_gles2.glTexStorageMem2DEXT(GL_TEXTURE_2D, 1, internalFormat, m_width,
                                     m_height, m_memoryObject, 0);
        s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
        s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
        m_BRSwizzle = true;
    } else {
        s_gles2.glTexStorageMem2DEXT(GL_TEXTURE_2D, 1, m_sizedInternalFormat, m_width, m_height, m_memoryObject, 0);
        m_BRSwizzle = false;
    }

    s_egl.eglDestroyImageKHR(m_display, m_eglImage);
    m_eglImage = s_egl.eglCreateImageKHR(
            m_display, s_egl.eglGetCurrentContext(), EGL_GL_TEXTURE_2D_KHR,
            (EGLClientBuffer)SafePointerFromUInt(m_tex), NULL);

    if (!vulkanOnly) {
        replaceContents(prevContents.data(), m_numBytes);
    }

    return true;
}

bool ColorBuffer::importEglNativePixmap(void* pixmap) {

    EGLImageKHR image = s_egl.eglCreateImageKHR(m_display, EGL_NO_CONTEXT, EGL_NATIVE_PIXMAP_KHR, pixmap, nullptr);

    if (image == EGL_NO_IMAGE_KHR) return false;

    // Assume pixmap is compatible with ColorBuffer's current dimensions and internal format.
    EGLBoolean setInfoRes = s_egl.eglSetImageInfoANDROID(m_display, image, m_width, m_height, m_internalFormat);

    if (EGL_TRUE != setInfoRes) {
        s_egl.eglDestroyImageKHR(m_display, image);
        return false;
    }

    rebindEglImage(image);
    return true;
}

bool ColorBuffer::importEglImage(void* nativeEglImage) {
    EGLImageKHR image = s_egl.eglImportImageANDROID(m_display, (EGLImage)nativeEglImage);

    if (image == EGL_NO_IMAGE_KHR) return false;

    // Assume nativeEglImage is compatible with ColorBuffer's current dimensions and internal format.
    EGLBoolean setInfoRes = s_egl.eglSetImageInfoANDROID(m_display, image, m_width, m_height, m_internalFormat);

    if (EGL_TRUE != setInfoRes) {
        s_egl.eglDestroyImageKHR(m_display, image);
        return false;
    }

    rebindEglImage(image);
    return true;
}

std::vector<uint8_t> ColorBuffer::getContentsAndClearStorage() {
    // Assume there is a current context.
    size_t bytes;
    readContents(&bytes, nullptr);
    std::vector<uint8_t> prevContents(bytes);
    readContents(&bytes, prevContents.data());
    s_gles2.glDeleteTextures(1, &m_tex);
    s_egl.eglDestroyImageKHR(m_display, m_eglImage);
    m_tex = 0;
    m_eglImage = (EGLImageKHR)0;
    return prevContents;
}

void ColorBuffer::restoreContentsAndEglImage(const std::vector<uint8_t>& contents, EGLImageKHR image) {
    s_gles2.glGenTextures(1, &m_tex);
    s_gles2.glBindTexture(GL_TEXTURE_2D, m_tex);

    m_eglImage = image;
    s_gles2.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)m_eglImage);

    replaceContents(contents.data(), m_numBytes);
}

void ColorBuffer::rebindEglImage(EGLImageKHR image) {
    RecursiveScopedHelperContext context(m_helper);
    auto contents = getContentsAndClearStorage();
    restoreContentsAndEglImage(contents, image);
}

void ColorBuffer::setInUse(bool inUse) {
    m_inUse = inUse;
}
