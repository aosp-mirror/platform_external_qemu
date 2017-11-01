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

#include "android/base/memory/ScopedPtr.h"

#include "DispatchTables.h"
#include "GLcommon/GLutils.h"
#include "RenderThreadInfo.h"
#include "TextureDraw.h"
#include "TextureResize.h"
#include "YUVConverter.h"

#include "OpenGLESDispatch/EGLDispatch.h"

#include <stdio.h>
#include <string.h>

namespace {

// Lazily create and bind a framebuffer object to the current host context.
// |fbo| is the address of the framebuffer object name.
// |tex| is the name of a texture that is attached to the framebuffer object
// on creation only. I.e. all rendering operations will target it.
// returns true in case of success, false on failure.
bool bindFbo(GLuint* fbo, GLuint tex) {
    if (*fbo) {
        // fbo already exist - just bind
        s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, *fbo);
        return true;
    }

    s_gles2.glGenFramebuffers(1, fbo);
    s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, *fbo);
    s_gles2.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0_OES,
                                   GL_TEXTURE_2D, tex, 0);
    GLenum status = s_gles2.glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE_OES) {
        ERR("ColorBuffer::bindFbo: FBO not complete: %#x\n", status);
        s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, 0);
        s_gles2.glDeleteFramebuffers(1, fbo);
        *fbo = 0;
        return false;
    }
    return true;
}

void unbindFbo() {
    s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

}

ColorBuffer::Helper::~Helper() = default;

// static
ColorBuffer* ColorBuffer::create(EGLDisplay p_display,
                                 int p_width,
                                 int p_height,
                                 GLenum p_internalFormat,
                                 FrameworkFormat p_frameworkFormat,
                                 HandleType hndl,
                                 Helper* helper) {
    GLenum texInternalFormat = 0;

    GLenum pixelType = GL_UNSIGNED_BYTE;
    int bytesPerPixel = 3;
    switch (p_internalFormat) {
        case GL_RGB:
            texInternalFormat = GL_RGB;
            bytesPerPixel = 3;
            break;

        case GL_RGB565_OES: // and GL_RGB565
            // Still say "GL_RGB" for compatibility
            // with older drivers
            texInternalFormat = GL_RGB;
            pixelType = GL_UNSIGNED_SHORT_5_6_5;
            bytesPerPixel = 2;
            break;

        case GL_RGBA:
        case GL_RGB5_A1_OES:
        case GL_RGBA4_OES:
            texInternalFormat = GL_RGBA;
            bytesPerPixel = 4;
            break;

        case GL_UNSIGNED_INT_10_10_10_2_OES:
            texInternalFormat = GL_RGBA;
            pixelType = GL_UNSIGNED_SHORT;
            bytesPerPixel = 4;
            break;

        case GL_RGBA16F:
            texInternalFormat = GL_RGBA;
            pixelType = GL_HALF_FLOAT;
            bytesPerPixel = 8;
            break;

        case GL_LUMINANCE:
            texInternalFormat = GL_LUMINANCE;
            pixelType = GL_UNSIGNED_SHORT;
            bytesPerPixel = 2;
            break;
        default:
            fprintf(stderr, "ColorBuffer::create invalid format 0x%x\n",
                    p_internalFormat);
            return NULL;
    }
    const unsigned long bufsize = ((unsigned long)bytesPerPixel) * p_width
            * p_height;
    android::base::ScopedCPtr<char> initialImage(
                static_cast<char*>(::malloc(bufsize)));
    if (!initialImage) {
        fprintf(stderr,
                "error: failed to allocate initial memory for ColorBuffer "
                "of size %dx%dx%d (%lu KB)\n",
                p_width, p_height, bytesPerPixel * 8, bufsize / 1024);
        return nullptr;
    }
    memset(initialImage.get(), 0xff, bufsize);

    RecursiveScopedHelperContext context(helper);
    if (!context.isOk()) {
        return NULL;
    }

    ColorBuffer* cb = new ColorBuffer(p_display, hndl, helper);

    s_gles2.glGenTextures(1, &cb->m_tex);
    s_gles2.glBindTexture(GL_TEXTURE_2D, cb->m_tex);

    s_gles2.glTexImage2D(GL_TEXTURE_2D, 0, texInternalFormat, p_width, p_height,
                         0, texInternalFormat, pixelType,
                         initialImage.get());
    initialImage.reset();

    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    //
    // create another texture for that colorbuffer for blit
    //
    s_gles2.glGenTextures(1, &cb->m_blitTex);
    s_gles2.glBindTexture(GL_TEXTURE_2D, cb->m_blitTex);
    s_gles2.glTexImage2D(GL_TEXTURE_2D, 0, texInternalFormat, p_width, p_height,
                         0, texInternalFormat, pixelType, NULL);

    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    cb->m_width = p_width;
    cb->m_height = p_height;
    cb->m_internalFormat = texInternalFormat;
    cb->m_format = texInternalFormat;
    cb->m_type = pixelType;

    cb->m_eglImage = s_egl.eglCreateImageKHR(
            p_display, s_egl.eglGetCurrentContext(), EGL_GL_TEXTURE_2D_KHR,
            (EGLClientBuffer)SafePointerFromUInt(cb->m_tex), NULL);

    cb->m_blitEGLImage = s_egl.eglCreateImageKHR(
            p_display, s_egl.eglGetCurrentContext(), EGL_GL_TEXTURE_2D_KHR,
            (EGLClientBuffer)SafePointerFromUInt(cb->m_blitTex), NULL);

    cb->m_resizer = new TextureResize(p_width, p_height);

    cb->m_frameworkFormat = p_frameworkFormat;
    switch (cb->m_frameworkFormat) {
        case FRAMEWORK_FORMAT_GL_COMPATIBLE:
            break;
        case FRAMEWORK_FORMAT_YV12:
        case FRAMEWORK_FORMAT_YUV_420_888:
            cb->m_yuv_converter.reset(
                    new YUVConverter(p_width, p_height, cb->m_frameworkFormat));
            break;
        default:
            break;
    }

    s_gles2.glFinish();
    return cb;
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

    m_yuv_converter.reset();

    GLuint tex[2] = {m_tex, m_blitTex};
    s_gles2.glDeleteTextures(2, tex);

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

    touch();
    if (bindFbo(&m_fbo, m_tex)) {
        s_gles2.glReadPixels(x, y, width, height, p_format, p_type, pixels);
        unbindFbo();
    }
}

void ColorBuffer::reformat(GLint internalformat,
                           GLenum format, GLenum type) {
    s_gles2.glBindTexture(GL_TEXTURE_2D, m_tex);
    s_gles2.glTexImage2D(GL_TEXTURE_2D, 0, internalformat, m_width, m_height,
                         0, format, type, nullptr);

    s_gles2.glBindTexture(GL_TEXTURE_2D, m_blitTex);
    s_gles2.glTexImage2D(GL_TEXTURE_2D, 0, internalformat, m_width, m_height,
                         0, format, type, nullptr);

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
    m_format = format;
    m_type = type;
}

void ColorBuffer::subUpdate(int x,
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

    touch();

    if (m_needFormatCheck) {
        if (p_type != m_type || p_format != m_format) {
            reformat((GLint)p_format, p_format, p_type);
        }
        m_needFormatCheck = false;
    }

    if (m_frameworkFormat == FRAMEWORK_FORMAT_YV12 ||
        m_frameworkFormat == FRAMEWORK_FORMAT_YUV_420_888) {
        assert(m_yuv_converter.get());

        // This FBO will convert the YUV frame to RGB
        // and render it to |m_tex|.
        bindFbo(&m_yuv_conversion_fbo, m_tex);
        m_yuv_converter->drawConvert(x, y, width, height, (char*)pixels);
        unbindFbo();

        // |m_tex| still needs to be bound afterwards
        s_gles2.glBindTexture(GL_TEXTURE_2D, m_tex);
    } else {
        s_gles2.glBindTexture(GL_TEXTURE_2D, m_tex);
        s_gles2.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        s_gles2.glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, p_format,
                                p_type, pixels);
    }
}

bool ColorBuffer::blitFromCurrentReadBuffer() {
    RenderThreadInfo* tInfo = RenderThreadInfo::get();
    if (!tInfo->currContext.get()) {
        // no Current context
        return false;
    }

    touch();
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

        // If the read buffer is multisampled, we need to resolve.
        GLint samples;
        s_gles2.glGetIntegerv(GL_SAMPLE_BUFFERS, &samples);
        if (tInfo->currContext->clientVersion() > GLESApi_2 && samples > 0) {
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
            s_gles2.glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, m_width,
                                        m_height);
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
        s_gles1.glGetIntegerv(GL_TEXTURE_BINDING_2D, &currTexBind);
        s_gles1.glGenTextures(1, &tmpTex);
        s_gles1.glBindTexture(GL_TEXTURE_2D, tmpTex);
        s_gles1.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_blitEGLImage);
        s_gles1.glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, m_width,
                                    m_height);
        s_gles1.glDeleteTextures(1, &tmpTex);
        s_gles1.glBindTexture(GL_TEXTURE_2D, currTexBind);
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
    return true;
}

GLuint ColorBuffer::scale() {
    RecursiveScopedHelperContext context(m_helper);
    touch();
    return m_resizer->update(m_tex);
}

bool ColorBuffer::post(GLuint tex, float rotation, float dx, float dy) {
    // NOTE: Do not call m_helper->setupContext() here!
    return m_helper->getTextureDraw()->draw(tex, rotation, dx, dy);
}

void ColorBuffer::readback(unsigned char* img) {
    RecursiveScopedHelperContext context(m_helper);
    if (!context.isOk()) {
        return;
    }
    touch();
    if (bindFbo(&m_fbo, m_tex)) {
        s_gles2.glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE,
                             img);
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
}

ColorBuffer* ColorBuffer::onLoad(android::base::Stream* stream,
                                 EGLDisplay p_display,
                                 Helper* helper) {
    HandleType hndl = static_cast<HandleType>(stream->getBe32());
    GLuint width = static_cast<GLuint>(stream->getBe32());
    GLuint height = static_cast<GLuint>(stream->getBe32());
    GLenum internalFormat = static_cast<GLenum>(stream->getBe32());
    FrameworkFormat frameworkFormat =
            static_cast<FrameworkFormat>(stream->getBe32());
    EGLImageKHR eglImage = reinterpret_cast<EGLImageKHR>(stream->getBe32());
    EGLImageKHR blitEGLImage = reinterpret_cast<EGLImageKHR>(stream->getBe32());

    if (!eglImage) {
        return create(p_display, width, height, internalFormat, frameworkFormat,
                      hndl, helper);
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
