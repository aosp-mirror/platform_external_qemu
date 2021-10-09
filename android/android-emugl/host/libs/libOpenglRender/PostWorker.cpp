/*
* Copyright (C) 2017 The Android Open Source Project
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
#include "PostWorker.h"

#include "ColorBuffer.h"
#include "DispatchTables.h"
#include "FrameBuffer.h"
#include "RenderThreadInfo.h"
#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "emugl/common/misc.h"

#include "emugl/common/misc.h"

#define POST_DEBUG 0
#if POST_DEBUG >= 1
#define DD(fmt, ...) \
    fprintf(stderr, "%s:%d| " fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define DD(fmt, ...) (void)0
#endif

static void sDefaultRunOnUiThread(UiUpdateFunc f, void* data, bool wait) {
    (void)f;
    (void)data;
    (void)wait;
}

PostWorker::PostWorker(
        PostWorker::BindSubwinCallback&& cb,
        bool mainThreadPostingOnly,
        EGLContext eglContext,
        EGLSurface eglSurface) :
    mFb(FrameBuffer::getFB()),
    mBindSubwin(cb),
    m_mainThreadPostingOnly(mainThreadPostingOnly),
    m_runOnUiThread(m_mainThreadPostingOnly ?
        emugl::get_emugl_window_operations().runOnUiThread :
        sDefaultRunOnUiThread),
    mContext(eglContext),
    mSurface(eglSurface) {}

void PostWorker::fillMultiDisplayPostStruct(ComposeLayer* l,
                                            hwc_rect_t displayArea,
                                            hwc_frect_t cropArea,
                                            hwc_transform_t transform) {
    l->composeMode = HWC2_COMPOSITION_DEVICE;
    l->blendMode = HWC2_BLEND_MODE_NONE;
    l->transform = transform;
    l->alpha = 1.0;
    l->displayFrame = displayArea;
    l->crop = cropArea;
}

void PostWorker::postImpl(HandleType cbHandle) {
    ColorBuffer* cb = mFb->findColorBuffer(cbHandle).get();
    if (cb == nullptr) {
        return;
    }
    // bind the subwindow eglSurface
    if (!m_mainThreadPostingOnly && !m_initialized) {
        m_initialized = mBindSubwin();
    }

    float dpr = mFb->getDpr();
    int windowWidth = mFb->windowWidth();
    int windowHeight = mFb->windowHeight();
    float px = mFb->getPx();
    float py = mFb->getPy();
    int zRot = mFb->getZrot();
    hwc_transform_t rotation = (hwc_transform_t)0;

    cb->waitSync();

    // Find the x and y values at the origin when "fully scrolled."
    // Multiply by 2 because the texture goes from -1 to 1, not 0 to 1.
    // Multiply the windowing coordinates by DPR because they ignore
    // DPR, but the viewport includes DPR.
    float fx = 2.f * (m_viewportWidth  - windowWidth  * dpr) / (float)m_viewportWidth;
    float fy = 2.f * (m_viewportHeight - windowHeight * dpr) / (float)m_viewportHeight;

    // finally, compute translation values
    float dx = px * fx;
    float dy = py * fy;

    if (emugl::get_emugl_multi_display_operations().isMultiDisplayEnabled()) {
        uint32_t combinedW, combinedH;
        emugl::get_emugl_multi_display_operations().getCombinedDisplaySize(&combinedW, &combinedH);
        mFb->getTextureDraw()->prepareForDrawLayer();
        int32_t start_id = -1, x, y;
        uint32_t id, w, h, c;
        while(emugl::get_emugl_multi_display_operations().getNextMultiDisplay(start_id, &id,
                                                                              &x, &y, &w, &h,
                                                                              nullptr, nullptr,
                                                                              &c)) {
            if ((id != 0) && (w == 0 || h == 0 || c == 0)) {
                start_id = id;
                continue;
            }
            ColorBuffer* multiDisplayCb = id == 0 ? cb : mFb->findColorBuffer(c).get();
            if (multiDisplayCb == nullptr) {
                start_id = id;
                continue;
            }
            ComposeLayer l;
            hwc_rect_t displayArea = { .left = (int)x,
                                       .top = (int)y,
                                       .right = (int)(x + w),
                                       .bottom = (int)(y + h) };
            hwc_frect_t cropArea = { .left = 0.0,
                                     .top = (float)multiDisplayCb->getHeight(),
                                     .right = (float)multiDisplayCb->getWidth(),
                                     .bottom = 0.0 };
            fillMultiDisplayPostStruct(&l, displayArea, cropArea, rotation);
            multiDisplayCb->postLayer(&l, combinedW, combinedH);
            start_id = id;
        }
        mFb->getTextureDraw()->cleanupForDrawLayer();
    }
    else if (emugl::get_emugl_window_operations().isFolded()) {
        mFb->getTextureDraw()->prepareForDrawLayer();
        ComposeLayer l;
        int x, y, w, h;
        emugl::get_emugl_window_operations().getFoldedArea(&x, &y, &w, &h);
        hwc_rect_t displayArea = { .left = 0,
                                   .top = 0,
                                   .right = windowWidth,
                                   .bottom = windowHeight };
        hwc_frect_t cropArea = { .left = (float)x,
                                 .top = (float)(y + h),
                                 .right = (float)(x + w),
                                 .bottom = (float)y };
        switch ((int)zRot/90) {
            case 1:
                rotation = HWC_TRANSFORM_ROT_270;
                break;
            case 2:
                rotation = HWC_TRANSFORM_ROT_180;
                break;
            case 3:
                rotation = HWC_TRANSFORM_ROT_90;
                break;
            default: ;
        }

        fillMultiDisplayPostStruct(&l, displayArea, cropArea, rotation);
        cb->postLayer(&l, m_viewportWidth/dpr, m_viewportHeight/dpr);
        mFb->getTextureDraw()->cleanupForDrawLayer();
    }
    else {
        // render the color buffer to the window and apply the overlay
        cb->postWithOverlay(cb->getTexture(), zRot, dx, dy);
        DD("post tex %dx%d to window %dx%d viewport %dx%d fb(%dx%d) dx %f dy %f\n",
            cb->getWidth(), cb->getHeight(), windowWidth, windowHeight,
            m_viewportWidth, m_viewportHeight,
            mFb->getWidth(), mFb->getHeight(),
            dx, dy);
    }

    s_egl.eglSwapBuffers(mFb->getDisplay(), mFb->getWindowSurface());
}

// Called whenever the subwindow needs a refresh (FrameBuffer::setupSubWindow).
// This rebinds the subwindow context (to account for
// when the refresh is a display change, for instance)
// and resets the posting viewport.
void PostWorker::viewportImpl(int width, int height) {
    // rebind the subwindow eglSurface unconditionally---
    // this could be from a display change
    if (!m_mainThreadPostingOnly) {
        m_initialized = mBindSubwin();
    }

    float dpr = mFb->getDpr();
    m_viewportWidth = width * dpr;
    m_viewportHeight = height * dpr;
    s_gles2.glViewport(0, 0, m_viewportWidth, m_viewportHeight);
}

// Called when the subwindow refreshes, but there is no
// last posted color buffer to show to the user. Instead of
// displaying whatever happens to be in the back buffer,
// clear() is useful for outputting consistent colors.
void PostWorker::clearImpl() {
#ifndef __linux__
    s_gles2.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                    GL_STENCIL_BUFFER_BIT);
    s_egl.eglSwapBuffers(mFb->getDisplay(), mFb->getWindowSurface());
#endif
}

void PostWorker::composeImpl(ComposeDevice* p) {
    // bind the subwindow eglSurface
    if (!m_mainThreadPostingOnly && !m_initialized) {
        m_initialized = mBindSubwin();
    }
    ComposeLayer* l = (ComposeLayer*)p->layer;
    GLint vport[4] = { 0, };
    s_gles2.glGetIntegerv(GL_VIEWPORT, vport);
    s_gles2.glViewport(0, 0, mFb->getWidth(),mFb->getHeight());
    if (!m_composeFbo) {
        s_gles2.glGenFramebuffers(1, &m_composeFbo);
    }
    s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, m_composeFbo);

    auto cbPtr = mFb->findColorBuffer(p->targetHandle);

    if (!cbPtr) {
        s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, 0);
        s_gles2.glViewport(vport[0], vport[1], vport[2], vport[3]);
        return;
    }

    s_gles2.glFramebufferTexture2D(GL_FRAMEBUFFER,
                                   GL_COLOR_ATTACHMENT0_OES,
                                   GL_TEXTURE_2D,
                                   cbPtr->getTexture(),
                                   0);

    DD("worker compose %d layers\n", p->numLayers);
    mFb->getTextureDraw()->prepareForDrawLayer();
    for (int i = 0; i < p->numLayers; i++, l++) {
        DD("\tcomposeMode %d color %d %d %d %d blendMode "
               "%d alpha %f transform %d %d %d %d %d "
               "%f %f %f %f\n",
               l->composeMode, l->color.r, l->color.g, l->color.b,
               l->color.a, l->blendMode, l->alpha, l->transform,
               l->displayFrame.left, l->displayFrame.top,
               l->displayFrame.right, l->displayFrame.bottom,
               l->crop.left, l->crop.top, l->crop.right,
               l->crop.bottom);
        composeLayer(l, mFb->getWidth(), mFb->getHeight());
    }

    cbPtr->setSync();

    s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, 0);
    s_gles2.glViewport(vport[0], vport[1], vport[2], vport[3]);
    mFb->getTextureDraw()->cleanupForDrawLayer();
}

void PostWorker::composev2Impl(ComposeDevice_v2* p) {
    // bind the subwindow eglSurface
    if (!m_mainThreadPostingOnly && !m_initialized) {
        m_initialized = mBindSubwin();
    }
    ComposeLayer* l = (ComposeLayer*)p->layer;
    GLint vport[4] = { 0, };
    s_gles2.glGetIntegerv(GL_VIEWPORT, vport);
    uint32_t w, h;
    emugl::get_emugl_multi_display_operations().getMultiDisplay(p->displayId,
                                                                nullptr,
                                                                nullptr,
                                                                &w,
                                                                &h,
                                                                nullptr,
                                                                nullptr,
                                                                nullptr);
    s_gles2.glViewport(0, 0, w, h);
    if (!m_composeFbo) {
        s_gles2.glGenFramebuffers(1, &m_composeFbo);
    }
    s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, m_composeFbo);

    auto cbPtr = mFb->findColorBuffer(p->targetHandle);

    if (!cbPtr) {
        s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, 0);
        s_gles2.glViewport(vport[0], vport[1], vport[2], vport[3]);
        return;
    }

    s_gles2.glFramebufferTexture2D(GL_FRAMEBUFFER,
                                   GL_COLOR_ATTACHMENT0_OES,
                                   GL_TEXTURE_2D,
                                   cbPtr->getTexture(),
                                   0);

    DD("worker compose %d layers\n", p->numLayers);
    mFb->getTextureDraw()->prepareForDrawLayer();
    for (int i = 0; i < p->numLayers; i++, l++) {
        DD("\tcomposeMode %d color %d %d %d %d blendMode "
               "%d alpha %f transform %d %d %d %d %d "
               "%f %f %f %f\n",
               l->composeMode, l->color.r, l->color.g, l->color.b,
               l->color.a, l->blendMode, l->alpha, l->transform,
               l->displayFrame.left, l->displayFrame.top,
               l->displayFrame.right, l->displayFrame.bottom,
               l->crop.left, l->crop.top, l->crop.right,
               l->crop.bottom);
        composeLayer(l, w, h);
    }

    cbPtr->setSync();
    s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, 0);
    s_gles2.glViewport(vport[0], vport[1], vport[2], vport[3]);
    mFb->getTextureDraw()->cleanupForDrawLayer();
}

void PostWorker::bind() {
    if (m_mainThreadPostingOnly) {
        if (mFb->getDisplay() != EGL_NO_DISPLAY) {
            EGLint res = s_egl.eglMakeCurrent(mFb->getDisplay(), mFb->getWindowSurface(), mFb->getWindowSurface(), mContext);
            if (!res) fprintf(stderr, "%s: error in binding: 0x%x\n", __func__, s_egl.eglGetError());
        } else {
            fprintf(stderr, "%s: no display!\n", __func__);
        }
    } else {
        mBindSubwin();
    }
}

void PostWorker::unbind() {
    if (mFb->getDisplay() != EGL_NO_DISPLAY) {
        s_egl.eglMakeCurrent(mFb->getDisplay(), EGL_NO_SURFACE, EGL_NO_SURFACE,
                             EGL_NO_CONTEXT);
    }
}

void PostWorker::composeLayer(ComposeLayer* l, uint32_t w, uint32_t h) {
    if (l->composeMode == HWC2_COMPOSITION_DEVICE) {
        ColorBufferPtr cb = mFb->findColorBuffer(l->cbHandle);
        if (!cb) {
            // bad colorbuffer handle
            // ERR("%s: fail to find colorbuffer %d\n", __FUNCTION__, l->cbHandle);
            return;
        }
        cb->postLayer(l, w, h);
    }
    else {
        // no Colorbuffer associated with SOLID_COLOR mode
        mFb->getTextureDraw()->drawLayer(l, w, h, 1, 1, 0);
    }
}

void PostWorker::screenshot(ColorBuffer* cb,
                            int width,
                            int height,
                            GLenum format,
                            GLenum type,
                            SkinRotation rotation,
                            void* pixels,
                            SkinRect rect) {
    cb->readPixelsScaled(width, height, format, type, rotation, pixels, rect);
}

PostWorker::~PostWorker() {
    if (mFb->getDisplay() != EGL_NO_DISPLAY) {
        s_egl.eglMakeCurrent(mFb->getDisplay(), EGL_NO_SURFACE, EGL_NO_SURFACE,
                             EGL_NO_CONTEXT);
    }
}

void PostWorker::post(HandleType cb) {
    if (m_mainThreadPostingOnly) {
        PostArgs args = {
            .postCb = cb,
        };

        m_toUiThread.send(args);

        m_runOnUiThread([](void* data) {
            PostWorker* p = (PostWorker*)data;
            PostArgs uiThreadArgs;
            p->m_toUiThread.receive(&uiThreadArgs);
            p->bind();
            p->postImpl(uiThreadArgs.postCb);
        },
        this,
        false /* no wait */);
    } else {
        postImpl(cb);
    }
}

void PostWorker::viewport(int width, int height) {
    if (m_mainThreadPostingOnly) {
        PostArgs args = {
            .width = width,
            .height = height,
        };

        m_toUiThread.send(args);

        m_runOnUiThread([](void* data) {
            PostWorker* p = (PostWorker*)data;
            PostArgs uiThreadArgs;
            p->m_toUiThread.receive(&uiThreadArgs);
            p->bind();
            p->viewportImpl(uiThreadArgs.width, uiThreadArgs.height);
        },
        this,
        false /* no wait */);
    } else {
        viewportImpl(width, height);
    }
}

void PostWorker::compose(ComposeDevice* p, uint32_t bufferSize) {
    if (m_mainThreadPostingOnly) {
        PostArgs args;
        std::vector<char> buffer(bufferSize, 0);
        memcpy(buffer.data(), p, bufferSize);
        args.composeBuffer = buffer;

        m_toUiThread.send(args);

        m_runOnUiThread([](void* data) {
            PostWorker* p = (PostWorker*)data;
            PostArgs uiThreadArgs;
            p->m_toUiThread.receive(&uiThreadArgs);
            p->bind();
            p->composeImpl((ComposeDevice*)uiThreadArgs.composeBuffer.data());
        },
        this,
        false /* no wait */);
    } else {
        composeImpl(p);
    }
}

void PostWorker::compose(ComposeDevice_v2* p, uint32_t bufferSize) {
    if (m_mainThreadPostingOnly) {
        PostArgs args;
        std::vector<char> buffer(bufferSize, 0);
        memcpy(buffer.data(), p, bufferSize);
        args.composeBuffer = buffer;

        m_toUiThread.send(args);

        m_runOnUiThread([](void* data) {
            PostWorker* p = (PostWorker*)data;
            PostArgs uiThreadArgs;
            p->m_toUiThread.receive(&uiThreadArgs);
            p->bind();
            p->composev2Impl((ComposeDevice_v2*)uiThreadArgs.composeBuffer.data());
        },
        this,
        false /* no wait */);
    } else {
        composev2Impl(p);
    }
}

void PostWorker::clear() {
    if (m_mainThreadPostingOnly) {
        PostArgs args = {
            .postCb = 0,
        };

        m_toUiThread.send(args);

        m_runOnUiThread([](void* data) {
            PostWorker* p = (PostWorker*)data;
            PostArgs uiThreadArgs;
            p->m_toUiThread.receive(&uiThreadArgs);
            p->bind();
            p->clearImpl();
        },
        this,
        false /* no wait */);
    } else {
        clearImpl();
    }
}

