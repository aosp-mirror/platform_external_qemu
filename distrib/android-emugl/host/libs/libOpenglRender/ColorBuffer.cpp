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

#include "EGLDispatch.h"
#include "FrameBuffer.h"
#include "GLESv1Dispatch.h"
#include "GLcommon/GLutils.h"
#ifdef WITH_GLES2
#include "GLESv2Dispatch.h"
#endif
#include "RenderThreadInfo.h"
#include <stdio.h>

ColorBuffer *ColorBuffer::create(int p_width, int p_height,
                                 GLenum p_internalFormat)
{
    FrameBuffer *fb = FrameBuffer::getFB();

    GLenum texInternalFormat = 0;

    switch(p_internalFormat) {
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

    if (!fb->bind_locked()) {
        return NULL;
    }

    ColorBuffer *cb = new ColorBuffer();


    s_gles1.glGenTextures(1, &cb->m_tex);
    s_gles1.glBindTexture(GL_TEXTURE_2D, cb->m_tex);
    int nComp = (texInternalFormat == GL_RGB ? 3 : 4);
    char *zBuff = new char[nComp*p_width*p_height];
    if (zBuff) {
        memset(zBuff, 0, nComp*p_width*p_height);
    }
    s_gles1.glTexImage2D(GL_TEXTURE_2D, 0, texInternalFormat,
                      p_width, p_height, 0,
                      texInternalFormat,
                      GL_UNSIGNED_BYTE, zBuff);
    delete [] zBuff;
    s_gles1.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    s_gles1.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    s_gles1.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    s_gles1.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    s_gles1.glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    //
    // create another texture for that colorbuffer for blit
    //
    s_gles1.glGenTextures(1, &cb->m_blitTex);
    s_gles1.glBindTexture(GL_TEXTURE_2D, cb->m_blitTex);
    s_gles1.glTexImage2D(GL_TEXTURE_2D, 0, texInternalFormat,
                      p_width, p_height, 0,
                      texInternalFormat,
                      GL_UNSIGNED_BYTE, NULL);
    s_gles1.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    s_gles1.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    s_gles1.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    s_gles1.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    s_gles1.glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    cb->m_width = p_width;
    cb->m_height = p_height;
    cb->m_internalFormat = texInternalFormat;

    if (fb->getCaps().has_eglimage_texture_2d) {
        cb->m_eglImage = s_egl.eglCreateImageKHR(
                fb->getDisplay(),
                s_egl.eglGetCurrentContext(),
                EGL_GL_TEXTURE_2D_KHR,
                (EGLClientBuffer)SafePointerFromUInt(cb->m_tex),
                NULL);

        cb->m_blitEGLImage = s_egl.eglCreateImageKHR(
                fb->getDisplay(),
                s_egl.eglGetCurrentContext(),
                EGL_GL_TEXTURE_2D_KHR,
                (EGLClientBuffer)SafePointerFromUInt(cb->m_blitTex),
                NULL);
    }

    fb->unbind_locked();
    return cb;
}

ColorBuffer::ColorBuffer() :
    m_tex(0),
    m_blitTex(0),
    m_eglImage(NULL),
    m_blitEGLImage(NULL),
    m_fbo(0),
    m_internalFormat(0)
{
}

ColorBuffer::~ColorBuffer()
{
    FrameBuffer *fb = FrameBuffer::getFB();
    fb->bind_locked();

    if (m_blitEGLImage) {
        s_egl.eglDestroyImageKHR(fb->getDisplay(), m_blitEGLImage);
    }
    if (m_eglImage) {
        s_egl.eglDestroyImageKHR(fb->getDisplay(), m_eglImage);
    }

    if (m_fbo) {
        s_gles1.glDeleteFramebuffersOES(1, &m_fbo);
    }

    GLuint tex[2] = {m_tex, m_blitTex};
    s_gles1.glDeleteTextures(2, tex);

    fb->unbind_locked();
}

void ColorBuffer::readPixels(int x, int y, int width, int height, GLenum p_format, GLenum p_type, void *pixels)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb->bind_locked()) return;
    if (bind_fbo()) {
        s_gles1.glReadPixels(x, y, width, height, p_format, p_type, pixels);
    }
    fb->unbind_locked();
}

void ColorBuffer::subUpdate(int x, int y, int width, int height, GLenum p_format, GLenum p_type, void *pixels)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb->bind_locked()) return;
    s_gles1.glBindTexture(GL_TEXTURE_2D, m_tex);
    s_gles1.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    s_gles1.glTexSubImage2D(GL_TEXTURE_2D, 0, x, y,
                         width, height, p_format, p_type, pixels);
    fb->unbind_locked();
}

bool ColorBuffer::blitFromCurrentReadBuffer()
{
    RenderThreadInfo *tInfo = RenderThreadInfo::get();
    if (!tInfo->currContext.Ptr()) {
        // no Current context
        return false;
    }

    //
    // Create a temporary texture inside the current context
    // from the blit_texture EGLImage and copy the pixels
    // from the current read buffer to that texture
    //
    GLuint tmpTex;
    GLint currTexBind;
    if (tInfo->currContext->isGL2()) {
        s_gles2.glGetIntegerv(GL_TEXTURE_BINDING_2D, &currTexBind);
        s_gles2.glGenTextures(1,&tmpTex);
        s_gles2.glBindTexture(GL_TEXTURE_2D, tmpTex);
        s_gles2.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_blitEGLImage);
        s_gles2.glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0,
                                  m_width, m_height);
    }
    else {
        s_gles1.glGetIntegerv(GL_TEXTURE_BINDING_2D, &currTexBind);
        s_gles1.glGenTextures(1,&tmpTex);
        s_gles1.glBindTexture(GL_TEXTURE_2D, tmpTex);
        s_gles1.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_blitEGLImage);
        s_gles1.glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0,
                                 m_width, m_height);
    }


    //
    // Now bind the frame buffer context and blit from
    // m_blitTex into m_tex
    //
    FrameBuffer *fb = FrameBuffer::getFB();
    if (fb->bind_locked()) {

        //
        // bind FBO object which has this colorbuffer as render target
        //
        if (bind_fbo()) {

            //
            // save current viewport and match it to the current
            // colorbuffer size
            //
            GLint vport[4] = {};
            s_gles1.glGetIntegerv(GL_VIEWPORT, vport);
            s_gles1.glViewport(0, 0, m_width, m_height);

            // render m_blitTex
            s_gles1.glBindTexture(GL_TEXTURE_2D, m_blitTex);
            s_gles1.glEnable(GL_TEXTURE_2D);
            s_gles1.glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            drawTexQuad();  // this will render the texture flipped

            // unbind the fbo
            s_gles1.glBindFramebufferOES(GL_FRAMEBUFFER_OES, 0);

            // restrore previous viewport
            s_gles1.glViewport(vport[0], vport[1], vport[2], vport[3]);
        }

        // unbind from the FrameBuffer context
        fb->unbind_locked();
    }

    //
    // delete the temporary texture and restore the texture binding
    // inside the current context
    //
    if (tInfo->currContext->isGL2()) {
        s_gles2.glDeleteTextures(1, &tmpTex);
        s_gles2.glBindTexture(GL_TEXTURE_2D, currTexBind);
    }
    else {
        s_gles1.glDeleteTextures(1, &tmpTex);
        s_gles1.glBindTexture(GL_TEXTURE_2D, currTexBind);
    }

    return true;
}

bool ColorBuffer::bindToTexture()
{
    if (m_eglImage) {
        RenderThreadInfo *tInfo = RenderThreadInfo::get();
        if (tInfo->currContext.Ptr()) {
#ifdef WITH_GLES2
            if (tInfo->currContext->isGL2()) {
                s_gles2.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_eglImage);
            }
            else {
                s_gles1.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_eglImage);
            }
#else
            s_gles1.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_eglImage);
#endif
            return true;
        }
    }
    return false;
}

bool ColorBuffer::bindToRenderbuffer()
{
    if (m_eglImage) {
        RenderThreadInfo *tInfo = RenderThreadInfo::get();
        if (tInfo->currContext.Ptr()) {
#ifdef WITH_GLES2
            if (tInfo->currContext->isGL2()) {
                s_gles2.glEGLImageTargetRenderbufferStorageOES(GL_RENDERBUFFER_OES, m_eglImage);
            }
            else {
                s_gles1.glEGLImageTargetRenderbufferStorageOES(GL_RENDERBUFFER_OES, m_eglImage);
            }
#else
            s_gles1.glEGLImageTargetRenderbufferStorageOES(GL_RENDERBUFFER_OES, m_eglImage);
#endif
            return true;
        }
    }
    return false;
}

bool ColorBuffer::bind_fbo()
{
    if (m_fbo) {
        // fbo already exist - just bind
        s_gles1.glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_fbo);
        return true;
    }

    s_gles1.glGenFramebuffersOES(1, &m_fbo);
    s_gles1.glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_fbo);
    s_gles1.glFramebufferTexture2DOES(GL_FRAMEBUFFER_OES,
                                   GL_COLOR_ATTACHMENT0_OES,
                                   GL_TEXTURE_2D, m_tex, 0);
    GLenum status = s_gles1.glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES);
    if (status != GL_FRAMEBUFFER_COMPLETE_OES) {
        ERR("ColorBuffer::bind_fbo: FBO not complete: %#x\n", status);
        s_gles1.glBindFramebufferOES(GL_FRAMEBUFFER_OES, 0);
        s_gles1.glDeleteFramebuffersOES(1, &m_fbo);
        m_fbo = 0;
        return false;
    }

    return true;
}

bool ColorBuffer::post()
{
    s_gles1.glBindTexture(GL_TEXTURE_2D, m_tex);
    s_gles1.glEnable(GL_TEXTURE_2D);
    s_gles1.glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    drawTexQuad();

    return true;
}

void ColorBuffer::drawTexQuad()
{
    GLfloat verts[] = { -1.0f, -1.0f, 0.0f,
                         -1.0f, +1.0f, 0.0f,
                         +1.0f, -1.0f, 0.0f,
                         +1.0f, +1.0f, 0.0f };

    GLfloat tcoords[] = { 0.0f, 1.0f,
                           0.0f, 0.0f,
                           1.0f, 1.0f,
                           1.0f, 0.0f };

    s_gles1.glClientActiveTexture(GL_TEXTURE0);
    s_gles1.glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    s_gles1.glTexCoordPointer(2, GL_FLOAT, 0, tcoords);

    s_gles1.glEnableClientState(GL_VERTEX_ARRAY);
    s_gles1.glVertexPointer(3, GL_FLOAT, 0, verts);
    s_gles1.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void ColorBuffer::readback(unsigned char* img)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (fb->bind_locked()) {
        if (bind_fbo()) {
            s_gles1.glReadPixels(0, 0, m_width, m_height,
                    GL_RGBA, GL_UNSIGNED_BYTE, img);
        }
        fb->unbind_locked();
    }
}
