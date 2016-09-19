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

#include "DispatchTables.h"
#include "GLcommon/GLutils.h"
#include "RenderThreadInfo.h"
#include "TextureDraw.h"
#include "TextureResize.h"

#include "OpenGLESDispatch/EGLDispatch.h"

#include <GLES3/gl31.h>
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
    s_gles2.glFramebufferTexture2D(GL_FRAMEBUFFER,
                                   GL_COLOR_ATTACHMENT0_OES,
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

// Helper class to use a ColorBuffer::Helper context.
// Usage is pretty simple:
//
//     {
//        ScopedHelperContext context(m_helper);
//        if (!context.isOk()) {
//            return false;   // something bad happened.
//        }
//        .... do something ....
//     }   // automatically calls m_helper->teardownContext();
//
class ScopedHelperContext {
public:
    ScopedHelperContext(ColorBuffer::Helper* helper) : mHelper(helper) {
        if (!helper->setupContext()) {
            mHelper = NULL;
        }
    }

    bool isOk() const { return mHelper != NULL; }

    ~ScopedHelperContext() {
        release();
    }

    void release() {
        if (mHelper) {
            mHelper->teardownContext();
            mHelper = NULL;
        }
    }
private:
    ColorBuffer::Helper* mHelper;
};

}  // namespace

// static
ColorBuffer* ColorBuffer::create(EGLDisplay p_display,
                                 int p_width,
                                 int p_height,
                                 GLenum p_internalFormat,
                                 bool has_eglimage_texture_2d,
                                 Helper* helper) {
    GLenum texInternalFormat = 0;

    switch (p_internalFormat) {
        case GL_RGB:
        case GL_RGB565_OES:
            texInternalFormat = GL_RGB;
            break;

        case GL_RGBA:
        case GL_RGB5_A1_OES:
        case GL_RGBA4_OES:
            texInternalFormat = GL_RGBA;
            break;

        default:
            return NULL;
            break;
    }

    ScopedHelperContext context(helper);
    if (!context.isOk()) {
        return NULL;
    }

    ColorBuffer *cb = new ColorBuffer(p_display, helper);

    s_gles2.glGenTextures(1, &cb->m_tex);
    s_gles2.glBindTexture(GL_TEXTURE_2D, cb->m_tex);

    int nComp = (texInternalFormat == GL_RGB ? 3 : 4);

    unsigned long bufsize = nComp * p_width * p_height;
    char* initialImage = static_cast<char*>(::calloc(bufsize, 1));
    memset(initialImage, 0xff, bufsize);

    s_gles2.glTexImage2D(GL_TEXTURE_2D,
                         0,
                         texInternalFormat,
                         p_width,
                         p_height,
                         0,
                         texInternalFormat,
                         GL_UNSIGNED_BYTE,
                         initialImage);
    ::free(initialImage);

    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    //
    // create another texture for that colorbuffer for blit
    //
    s_gles2.glGenTextures(1, &cb->m_blitTex);
    s_gles2.glBindTexture(GL_TEXTURE_2D, cb->m_blitTex);
    s_gles2.glTexImage2D(GL_TEXTURE_2D,
                         0,
                         texInternalFormat,
                         p_width,
                         p_height,
                         0,
                         texInternalFormat,
                         GL_UNSIGNED_BYTE,
                         NULL);

    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    cb->m_width = p_width;
    cb->m_height = p_height;
    cb->m_internalFormat = texInternalFormat;

    if (has_eglimage_texture_2d) {
        cb->m_eglImage = s_egl.eglCreateImageKHR(
                p_display,
                s_egl.eglGetCurrentContext(),
                EGL_GL_TEXTURE_2D_KHR,
                (EGLClientBuffer)SafePointerFromUInt(cb->m_tex),
                NULL);

        cb->m_blitEGLImage = s_egl.eglCreateImageKHR(
                p_display,
                s_egl.eglGetCurrentContext(),
                EGL_GL_TEXTURE_2D_KHR,
                (EGLClientBuffer)SafePointerFromUInt(cb->m_blitTex),
                NULL);
    }

    cb->m_resizer = new TextureResize(p_width, p_height);

    fprintf(stderr, "%s: probabyl should initialize PBOs here\n", __FUNCTION__);
    s_gles2.glGenBuffers(1, &cb->pbos[0]);
    s_gles2.glBindBuffer(GL_PIXEL_UNPACK_BUFFER, cb->pbos[0]);
    s_gles2.glBufferData(GL_PIXEL_UNPACK_BUFFER, bufsize, 0, GL_STREAM_DRAW);
    s_gles2.glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    fprintf(stderr, "%s: created pbo %u\n", __FUNCTION__, cb->pbos[0]);

    s_gles2.glFinish();
    return cb;
}

ColorBuffer::ColorBuffer(EGLDisplay display, Helper* helper) :
        m_display(display),
        m_helper(helper) {}

ColorBuffer::~ColorBuffer() {
    ScopedHelperContext context(m_helper);

    fprintf(stderr, "%s: deleting pbo %u\n", __FUNCTION__, pbos[0]);
    s_gles2.glDeleteBuffers(1, &pbos[0]);

    if (m_blitEGLImage) {
        s_egl.eglDestroyImageKHR(m_display, m_blitEGLImage);
    }
    if (m_eglImage) {
        s_egl.eglDestroyImageKHR(m_display, m_eglImage);
    }

    if (m_fbo) {
        s_gles2.glDeleteFramebuffers(1, &m_fbo);
    }

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
    ScopedHelperContext context(m_helper);
    if (!context.isOk()) {
        return;
    }

    if (bindFbo(&m_fbo, m_tex)) {
        s_gles2.glReadPixels(x, y, width, height, p_format, p_type, pixels);
        unbindFbo();
    }
}

// static void getErr() {
//     GLint err = s_gles2.glGetError();
//     if (err != GL_NO_ERROR) {
//         fprintf(stderr, "%s: detected err 0x%x\n", __FUNCTION__, err);
//     }
// }


#include <sys/time.h>

static uint64_t currTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * ((uint64_t)1000000) + tv.tv_usec) / 1000;
}
void ColorBuffer::subUpdate(int x,
                            int y,
                            int width,
                            int height,
                            GLenum p_format,
                            GLenum p_type,
                            void* pixels) {
    uint64_t start = currTime();

    ScopedHelperContext context(m_helper);
    if (!context.isOk()) {
        return;
    }

    s_gles2.glBindTexture(GL_TEXTURE_2D, m_tex);
    s_gles2.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // s_gles2.glTexSubImage2D(
    //         GL_TEXTURE_2D, 0, x, y, width, height, p_format, p_type, pixels);
   

    uint32_t bpp = 4;
    switch (p_format) {
    case GL_RGBA:
        // fprintf(stderr, "%s: update GL_RGBA 4 bpp type 0x%x\n", __FUNCTION__, p_type);
        bpp = 4;
        break;
    case GL_RGB:
        // fprintf(stderr, "%s: update GL_RGB 3 bpp type 0x%x\n", __FUNCTION__, p_type);
        bpp = 3;
        break;
    default:
        fprintf(stderr, "%s: update some other format 0x%x type 0x%x\n", __FUNCTION__, p_format, p_type);
    }

    GLsizeiptr numBytesTotal = bpp * width * height;
    if (!numBytesTotal) return;

    // fprintf(stderr, "%s: numBytes %u w h %d %d\n", __FUNCTION__, (uint32_t)numBytesTotal, width, height); fprintf(stderr, "%s: errcheck.\n", __FUNCTION__); getErr(); fprintf(stderr, "%s: writing device memory.\n", __FUNCTION__);
    s_gles2.glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos[0]);
    //getErr();
    // fprintf(stderr, "%s: bound unpack buffer. reset current data\n", __FUNCTION__); fprintf(stderr, "%s: alloced buffer data.\n", __FUNCTION__);
    GLubyte* device_ptr = (GLubyte*)s_gles2.glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, numBytesTotal, GL_MAP_WRITE_BIT); //getErr();
    if (device_ptr) {
        // fprintf(stderr, "%s: mapped range %p..memcpying\n", __FUNCTION__, device_ptr);
        memcpy(device_ptr, pixels, numBytesTotal);
        // fprintf(stderr, "%s: successfuly wrote to device memory.\n", __FUNCTION__);
        s_gles2.glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); 
        //getErr();
        // fprintf(stderr, "%s: unmapped range. teximage...\n", __FUNCTION__);
        s_gles2.glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, p_format, p_type, 0);
    }
    s_gles2.glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0); 
    //getErr();

    uint64_t end = currTime() - start;
    fprintf(stderr, "%s: elapsed %llu\n", __FUNCTION__, (unsigned long long)end);
}

bool ColorBuffer::blitFromCurrentReadBuffer()
{
    RenderThreadInfo *tInfo = RenderThreadInfo::get();
    if (!tInfo->currContext.get()) {
        // no Current context
        return false;
    }

    // Copy the content of the current read surface into m_blitEGLImage.
    // This is done by creating a temporary texture, bind it to the EGLImage
    // then call glCopyTexSubImage2D().
    GLuint tmpTex;
    GLint currTexBind;
    if (tInfo->currContext->isGL2()) {
        s_gles2.glGetIntegerv(GL_TEXTURE_BINDING_2D, &currTexBind);
        s_gles2.glGenTextures(1,&tmpTex);
        s_gles2.glBindTexture(GL_TEXTURE_2D, tmpTex);
        s_gles2.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_blitEGLImage);
        s_gles2.glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0,
                                  m_width, m_height);
        s_gles2.glDeleteTextures(1, &tmpTex);
        s_gles2.glBindTexture(GL_TEXTURE_2D, currTexBind);
    }
    else {
        s_gles1.glGetIntegerv(GL_TEXTURE_BINDING_2D, &currTexBind);
        s_gles1.glGenTextures(1,&tmpTex);
        s_gles1.glBindTexture(GL_TEXTURE_2D, tmpTex);
        s_gles1.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_blitEGLImage);
        s_gles1.glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0,
                                 m_width, m_height);
        s_gles1.glDeleteTextures(1, &tmpTex);
        s_gles1.glBindTexture(GL_TEXTURE_2D, currTexBind);
    }

    ScopedHelperContext context(m_helper);
    if (!context.isOk()) {
        return false;
    }

    if (!bindFbo(&m_fbo, m_tex)) {
        return false;
    }

    // Save current viewport and match it to the current colorbuffer size.
    GLint vport[4] = { 0, };
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
    RenderThreadInfo *tInfo = RenderThreadInfo::get();
    if (!tInfo->currContext.get()) {
        return false;
    }
    if (tInfo->currContext->isGL2()) {
        s_gles2.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_eglImage);
    }
    else {
        s_gles1.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_eglImage);
    }
    return true;
}

bool ColorBuffer::bindToRenderbuffer() {
    if (!m_eglImage) {
        return false;
    }
    RenderThreadInfo *tInfo = RenderThreadInfo::get();
    if (!tInfo->currContext.get()) {
        return false;
    }
    if (tInfo->currContext->isGL2()) {
        s_gles2.glEGLImageTargetRenderbufferStorageOES(GL_RENDERBUFFER_OES, m_eglImage);
    }
    else {
        s_gles1.glEGLImageTargetRenderbufferStorageOES(GL_RENDERBUFFER_OES, m_eglImage);
    }
    return true;
}

bool ColorBuffer::post(float rotation, float dx, float dy) {
    // NOTE: Do not call m_helper->setupContext() here!
    return m_helper->getTextureDraw()->draw(m_resizer->update(m_tex), rotation, dx, dy);
}

void ColorBuffer::readback(unsigned char* img) {
    ScopedHelperContext context(m_helper);
    if (!context.isOk()) {
        return;
    }
    if (bindFbo(&m_fbo, m_tex)) {
        s_gles2.glReadPixels(
                0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, img);
        unbindFbo();
    }
}
