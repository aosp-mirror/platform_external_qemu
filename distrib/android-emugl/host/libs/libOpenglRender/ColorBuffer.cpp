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

#include <stdio.h>
#define RC_ERRCHECK
#ifdef RC_ERRCHECK
#define ERRCHECK() do { \
    EGLint egl_errcode = s_egl.eglGetError(); \
    if (egl_errcode != EGL_SUCCESS) { \
        fprintf(stderr, "%s: ", __FUNCTION__); \
        fprintf(stderr, "colorbuffer: eglerror=0x%x\n", egl_errcode); \
    } \
    GLint gl_errcode = s_gles2.glGetError(); \
    if (gl_errcode != GL_NO_ERROR) { \
        fprintf(stderr, "colorbuffer: gles2error=0x%x\n", gl_errcode); \
    } \
} while(0) 
#define LASTGLCALL_CHECK(...) do { \
    fprintf(stderr, "colorbuffer: %s:%d lastcall: ", __FUNCTION__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, "\n"); \
    ERRCHECK(); \
} while(0)
#else
#define LASTGLCALL_CHECK(...) 0
#define ERRCHECK() 0
#endif
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

    s_gles2.glGenFramebuffers(1, fbo); LASTGLCALL_CHECK("glGenFramebuffers(1, *)");
    s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, *fbo); LASTGLCALL_CHECK("glBindFramebuffer(GL_FRAMEBUFFER, *)");
    s_gles2.glFramebufferTexture2D(GL_FRAMEBUFFER,
                                   GL_COLOR_ATTACHMENT0_OES,
                                   GL_TEXTURE_2D, tex, 0); LASTGLCALL_CHECK("glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0_OES, GL_TEXTURE_2D, %d, 0)", tex);
    GLenum status = s_gles2.glCheckFramebufferStatus(GL_FRAMEBUFFER); LASTGLCALL_CHECK("glCheckFramebufferStatus(GL_FRAMEBUFFER)");
    if (status != GL_FRAMEBUFFER_COMPLETE_OES) {
        ERR("ColorBuffer::bindFbo: FBO not complete: %#x\n", status);
        s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, 0); LASTGLCALL_CHECK("glBindFramebuffer(GL_FRAMEBUFFER, 0)");
        s_gles2.glDeleteFramebuffers(1, fbo); LASTGLCALL_CHECK("glDeleteFramebuffers(GL_FRAMEBUFFER, 0)");
        *fbo = 0;
        return false;
    }
    return true;
}

void unbindFbo() {
    s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, 0); LASTGLCALL_CHECK("glBindFramebuffer(GL_FRAMEBUFFER, 0)");
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

    s_gles2.glGenTextures(1, &cb->m_tex); LASTGLCALL_CHECK("glGenTextures(1, *)");
    s_gles2.glBindTexture(GL_TEXTURE_2D, cb->m_tex); LASTGLCALL_CHECK("glBindTexture(GL_TEXTURE_2D, *)");

    int nComp = (texInternalFormat == GL_RGB ? 3 : 4);

    char* zBuff = static_cast<char*>(::calloc(nComp * p_width * p_height, 1));
    s_gles2.glTexImage2D(GL_TEXTURE_2D,
                         0,
                         texInternalFormat,
                         p_width,
                         p_height,
                         0,
                         texInternalFormat,
                         GL_UNSIGNED_BYTE,
                         zBuff); LASTGLCALL_CHECK("glTexImage2D(GL_TEXTURE_2D, 0, %d, %d, %d, 0, %d, GL_UNSIGNED_BYTE, *)", texInternalFormat, p_width, p_height, texInternalFormat);
    ::free(zBuff);

    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    LASTGLCALL_CHECK("glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)");
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    LASTGLCALL_CHECK("glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)");
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    LASTGLCALL_CHECK("glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)");
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    LASTGLCALL_CHECK("glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)");

    //
    // create another texture for that colorbuffer for blit
    //
    s_gles2.glGenTextures(1, &cb->m_blitTex);
    LASTGLCALL_CHECK("glGenTextures(1, &cb->m_blitTex);");
    s_gles2.glBindTexture(GL_TEXTURE_2D, cb->m_blitTex);
    LASTGLCALL_CHECK("glBindTexture(GL_TEXTURE_2D, cb->m_blitTex);");
    s_gles2.glTexImage2D(GL_TEXTURE_2D,
                         0,
                         texInternalFormat,
                         p_width,
                         p_height,
                         0,
                         texInternalFormat,
                         GL_UNSIGNED_BYTE,
                         NULL);
    LASTGLCALL_CHECK("s_gles2.glTexImage2D(GL_TEXTURE_2D, 0, %d, %d, %d, 0, %d, GL_UNSIGNED_BYTE, NULL);", texInternalFormat, p_width, p_height, texInternalFormat);

    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    LASTGLCALL_CHECK("glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);");
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    LASTGLCALL_CHECK("glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);");
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    LASTGLCALL_CHECK("glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);");
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    LASTGLCALL_CHECK("glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);");

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
        LASTGLCALL_CHECK("eglCreateImageKHR( p_display, s_egl.eglGetCurrentContext(), EGL_GL_TEXTURE_2D_KHR, (EGLClientBuffer)SafePointerFromUInt(cb->m_tex), NULL);");

        cb->m_blitEGLImage = s_egl.eglCreateImageKHR(
                p_display,
                s_egl.eglGetCurrentContext(),
                EGL_GL_TEXTURE_2D_KHR,
                (EGLClientBuffer)SafePointerFromUInt(cb->m_blitTex),
                NULL);
        LASTGLCALL_CHECK("eglCreateImageKHR( p_display, s_egl.eglGetCurrentContext(), EGL_GL_TEXTURE_2D_KHR, (EGLClientBuffer)SafePointerFromUInt(cb->m_blitTex), NULL);");

        if (!cb->m_eglImage) { LASTGLCALL_CHECK("ERROR: CANNOT CREATE m_eglImage"); }
        if (!cb->m_blitEGLImage) { LASTGLCALL_CHECK("ERROR: CANNOT CREATE m_eglImage"); }
    }

    cb->m_resizer = new TextureResize(p_width, p_height);

    return cb;
}

ColorBuffer::ColorBuffer(EGLDisplay display, Helper* helper) :
        m_tex(0),
        m_blitTex(0),
        m_eglImage(NULL),
        m_blitEGLImage(NULL),
        m_fbo(0),
        m_internalFormat(0),
        m_display(display),
        m_helper(helper) {}

ColorBuffer::~ColorBuffer() {
    ScopedHelperContext context(m_helper);

    if (m_blitEGLImage) {
        s_egl.eglDestroyImageKHR(m_display, m_blitEGLImage);
        LASTGLCALL_CHECK("eglDestroyImageKHR(m_display, m_blitEGLImage);");
    }
    if (m_eglImage) {
        s_egl.eglDestroyImageKHR(m_display, m_eglImage);
        LASTGLCALL_CHECK("eglDestroyImageKHR(m_display, m_eglImage);");
    }

    if (m_fbo) {
        s_gles2.glDeleteFramebuffers(1, &m_fbo);
        LASTGLCALL_CHECK("glDeleteFramebuffers(1, &m_fbo);");
    }

    GLuint tex[2] = {m_tex, m_blitTex};
    s_gles2.glDeleteTextures(2, tex);
    LASTGLCALL_CHECK("glDeleteTextures(2, tex);");

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
        LASTGLCALL_CHECK("glReadPixels(%d, %d, %d, %d, %d, %d, pixels);",x,y,width,height,p_format,p_type);
        unbindFbo();
    }
}

void ColorBuffer::subUpdate(int x,
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

    s_gles2.glBindTexture(GL_TEXTURE_2D, m_tex);
    LASTGLCALL_CHECK("glBindTexture(GL_TEXTURE_2D, m_tex);");
    s_gles2.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    LASTGLCALL_CHECK("glPixelStorei(GL_UNPACK_ALIGNMENT, 1);");
    s_gles2.glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, p_format, p_type, pixels);
    LASTGLCALL_CHECK("glTexSubImage2D(GL_TEXTURE_2D, 0, %d, %d, %d, %d, %d, %d, pixels);",
                     x,y,width,height,p_format,p_type);
}

bool ColorBuffer::blitFromCurrentReadBuffer()
{
    RenderThreadInfo *tInfo = RenderThreadInfo::get();
    if (!tInfo->currContext.Ptr()) {
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
        LASTGLCALL_CHECK("glGetIntegerv(GL_TEXTURE_BINDING_2D, &currTexBind);");
        s_gles2.glGenTextures(1,&tmpTex);
        LASTGLCALL_CHECK("glGenTextures(1,&tmpTex);");
        s_gles2.glBindTexture(GL_TEXTURE_2D, tmpTex);
        LASTGLCALL_CHECK("glBindTexture(GL_TEXTURE_2D, tmpTex);");
        s_gles2.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_blitEGLImage);
        LASTGLCALL_CHECK("glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_blitEGLImage);");
        s_gles2.glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, m_width, m_height);
        LASTGLCALL_CHECK("glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, m_width, m_height);");
        s_gles2.glDeleteTextures(1, &tmpTex);
        LASTGLCALL_CHECK("glDeleteTextures(1, &tmpTex);");
        s_gles2.glBindTexture(GL_TEXTURE_2D, currTexBind);
        LASTGLCALL_CHECK("glBindTexture(GL_TEXTURE_2D, currTexBind);");
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
    LASTGLCALL_CHECK("glGetIntegerv(GL_VIEWPORT, vport);");
    s_gles2.glViewport(0, 0, m_width, m_height);
    LASTGLCALL_CHECK("glViewport(0, 0, m_width, m_height);");

    // render m_blitTex
    m_helper->getTextureDraw()->draw(m_blitTex, 0., 0, 0);

    // Restore previous viewport.
    s_gles2.glViewport(vport[0], vport[1], vport[2], vport[3]);
    LASTGLCALL_CHECK("glViewport(vport[0], vport[1], vport[2], vport[3]);");
    unbindFbo();

    return true;
}

bool ColorBuffer::bindToTexture() {
    if (!m_eglImage) {
        return false;
    }
    RenderThreadInfo *tInfo = RenderThreadInfo::get();
    if (!tInfo->currContext.Ptr()) {
        return false;
    }
    if (tInfo->currContext->isGL2()) {
        s_gles2.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_eglImage);
        LASTGLCALL_CHECK("glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_eglImage);");
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
    if (!tInfo->currContext.Ptr()) {
        return false;
    }
    if (tInfo->currContext->isGL2()) {
        s_gles2.glEGLImageTargetRenderbufferStorageOES(GL_RENDERBUFFER_OES, m_eglImage);
        LASTGLCALL_CHECK("glEGLImageTargetRenderbufferStorageOES(GL_RENDERBUFFER_OES, m_eglImage);");
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
        s_gles2.glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, img);
        LASTGLCALL_CHECK("glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, img);");
        unbindFbo();
    }
}
