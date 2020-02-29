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

void PostWorker::fillMultiDisplayPostStruct(ComposeLayer* l, int idx, ColorBuffer* cb) {
    l->composeMode = HWC2_COMPOSITION_DEVICE;
    l->blendMode = HWC2_BLEND_MODE_NONE;
    l->transform = (hwc_transform_t)0;
    l->alpha = 1.0;
    l->displayFrame.left = mFb->m_displays[idx].pos_x;
    l->displayFrame.top = mFb->m_displays[idx].pos_y;
    l->displayFrame.right = mFb->m_displays[idx].pos_x + mFb->m_displays[idx].width;
    l->displayFrame.bottom =  mFb->m_displays[idx].pos_y + mFb->m_displays[idx].height;
    l->crop.left = 0.0;
    l->crop.top = (float)cb->getHeight();
    l->crop.right = (float)cb->getWidth();
    l->crop.bottom = 0.0;
}

void PostWorker::postImpl(ColorBuffer* cb) {
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

    if (mFb->m_displays.size() > 1 ) {
        int combinedW, combinedH;
        mFb->getCombinedDisplaySize(&combinedW, &combinedH);
        mFb->getTextureDraw()->prepareForDrawLayer();
        for (const auto& iter : mFb->m_displays) {
            if ((iter.first != 0) &&
                (iter.second.width == 0 || iter.second.height == 0 || iter.second.cb == 0)) {
                continue;
            }
            // don't render 2nd cb to primary display
            if (iter.first == 0 && cb->getDisplay() != 0) {
                continue;
            }
            ColorBuffer* multiDisplayCb = iter.first == 0 ? cb :
                mFb->findColorBuffer(iter.second.cb).get();
            if (multiDisplayCb == nullptr) {
                ERR("fail to find cb %d\n", iter.second.cb);
                continue;
            }
            ComposeLayer l;
            fillMultiDisplayPostStruct(&l, iter.first, multiDisplayCb);
            multiDisplayCb->postLayer(&l, combinedW, combinedH);
        }
    } else {
        // render the color buffer to the window and apply the overlay
        GLuint tex = cb->scale();
        cb->postWithOverlay(tex, zRot, dx, dy);
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
    s_gles2.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                    GL_STENCIL_BUFFER_BIT);
    s_egl.eglSwapBuffers(mFb->getDisplay(), mFb->getWindowSurface());
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
    s_gles2.glFramebufferTexture2D(GL_FRAMEBUFFER,
                                   GL_COLOR_ATTACHMENT0_OES,
                                   GL_TEXTURE_2D,
                                   mFb->findColorBuffer(p->targetHandle)->getTexture(),
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
        composeLayer(l);
    }

    mFb->findColorBuffer(p->targetHandle)->setSync();
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
    s_gles2.glViewport(0, 0, mFb->getWidth(),mFb->getHeight());
    if (!m_composeFbo) {
        s_gles2.glGenFramebuffers(1, &m_composeFbo);
    }
    s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, m_composeFbo);
    s_gles2.glFramebufferTexture2D(GL_FRAMEBUFFER,
                                   GL_COLOR_ATTACHMENT0_OES,
                                   GL_TEXTURE_2D,
                                   mFb->findColorBuffer(p->targetHandle)->getTexture(),
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
        composeLayer(l);
    }

    mFb->findColorBuffer(p->targetHandle)->setSync();
    s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, 0);
    s_gles2.glViewport(vport[0], vport[1], vport[2], vport[3]);
    mFb->getTextureDraw()->cleanupForDrawLayer();
}

void PostWorker::bind() {
    if (m_mainThreadPostingOnly) {
        if (mFb->getDisplay() != EGL_NO_DISPLAY) {
            fprintf(stderr, "%s: bind surf %p context %p\n", __func__, mFb->getWindowSurface(), mContext);
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

void PostWorker::composeLayer(ComposeLayer* l) {
    if (l->composeMode == HWC2_COMPOSITION_DEVICE) {
        ColorBufferPtr cb = mFb->findColorBuffer(l->cbHandle);
        if (cb == nullptr) {
            // bad colorbuffer handle
            ERR("%s: fail to find colorbuffer %d\n", __FUNCTION__, l->cbHandle);
            return;
        }
        cb->postLayer(l, mFb->getWidth(), mFb->getHeight());
    }
    else {
        // no Colorbuffer associated with SOLID_COLOR mode
        mFb->getTextureDraw()->drawLayer(l, mFb->getWidth(), mFb->getHeight(),
                                         1, 1, 0);
    }
}

PostWorker::~PostWorker() {
    if (mFb->getDisplay() != EGL_NO_DISPLAY) {
        s_egl.eglMakeCurrent(mFb->getDisplay(), EGL_NO_SURFACE, EGL_NO_SURFACE,
                             EGL_NO_CONTEXT);
    }
}

void PostWorker::post(ColorBuffer* cb) {
    if (m_mainThreadPostingOnly) {
        fprintf(stderr, "%s: main thread posting only\n", __func__);
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
            p->m_fromUiThread.send(0);
        },
        this,
        false /* no wait */);

        int response;
        m_fromUiThread.receive(&response);

        fprintf(stderr, "%s: main thread posting only (done)\n", __func__);
    } else {
        postImpl(cb);
    }
}

void PostWorker::viewport(int width, int height) {
    if (m_mainThreadPostingOnly) {
        fprintf(stderr, "%s: main thread posting only\n", __func__);
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
            p->m_fromUiThread.send(0);
        },
        this,
        false /* no wait */);

        int response;
        m_fromUiThread.receive(&response);
        fprintf(stderr, "%s: main thread posting only (done)\n", __func__);

    } else {
        viewport(width, height);
    }
}

void PostWorker::compose(ComposeDevice* p) {
    if (m_mainThreadPostingOnly) {
        fprintf(stderr, "%s: main thread posting only\n", __func__);
        PostArgs args = {
            .composeDevice = p,
        };

        m_toUiThread.send(args);

        m_runOnUiThread([](void* data) {
            PostWorker* p = (PostWorker*)data;
            PostArgs uiThreadArgs;
            p->m_toUiThread.receive(&uiThreadArgs);
            p->bind();
            p->composeImpl(uiThreadArgs.composeDevice);
            p->m_fromUiThread.send(0);
        },
        this,
        false /* no wait */);

        int response;
        m_fromUiThread.receive(&response);
        fprintf(stderr, "%s: main thread posting only (done)\n", __func__);
    } else {
        compose(p);
    }
}

void PostWorker::compose(ComposeDevice_v2* p) {
    if (m_mainThreadPostingOnly) {
        fprintf(stderr, "%s: main thread posting only\n", __func__);
        PostArgs args = {
            .composeDevice_v2 = p,
        };

        m_toUiThread.send(args);

        m_runOnUiThread([](void* data) {
            PostWorker* p = (PostWorker*)data;
            PostArgs uiThreadArgs;
            p->m_toUiThread.receive(&uiThreadArgs);
            p->bind();
            p->composev2Impl(uiThreadArgs.composeDevice_v2);
            p->m_fromUiThread.send(0);
        },
        this,
        false /* no wait */);

        int response;
        m_fromUiThread.receive(&response);
        fprintf(stderr, "%s: main thread posting only (done)\n", __func__);
    } else {
        compose(p);
    }
}

void PostWorker::clear() {
    if (m_mainThreadPostingOnly) {
        fprintf(stderr, "%s: main thread posting only\n", __func__);
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
            p->m_fromUiThread.send(0);
        },
        this,
        false /* no wait */);

        int response;
        m_fromUiThread.receive(&response);
        fprintf(stderr, "%s: main thread posting only (done)\n", __func__);
    } else {
        clear();
    }
}

