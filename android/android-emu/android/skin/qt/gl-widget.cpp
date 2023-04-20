// Copyright 2016 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/gl-widget.h"

#include "android/opengles.h"

#include <qloggingcategory.h>                   // for qCWarning
#include <qnamespace.h>                         // for WA_DontCreateNativeAn...
#include <QRegion>                              // for QRegion
#include <QResizeEvent>                         // for QResizeEvent
#include <QSize>                                // for QSize
#include <QWindow>                              // for QWindow

#include "EGL/egl.h"                            // for EGL_FALSE, EGL_NONE
#include "EGL/eglplatform.h"                    // for EGLint, EGLNativeWind...
#include "GLES3/gl3.h"                          // for GL_TEXTURE_2D
#include "OpenGLESDispatch/EGLDispatch.h"       // for EGLDispatch
#include "OpenGLESDispatch/GLESv2Dispatch.h"    // for GLESv2Dispatch
#include "android/skin/qt/gl-canvas.h"          // for GLCanvas
#include "android/skin/qt/gl-texture-draw.h"    // for TextureDraw
#include "android/skin/qt/logging-category.h"   // for emu
#include "emugl/common/OpenGLDispatchLoader.h"  // for LazyLoadedEGLDispatch

class QPaintEvent;
class QResizeEvent;
class QScreen;
class QShowEvent;
class QWidget;

struct EGLState {
    EGLDisplay display;
    EGLContext context;
    EGLSurface surface;
};

// Helper function to get the nearest power-of-two.
// It's needed to create a GLCanvas with correct dimensions
// to allow generating mipmaps (GLES 2.0 doesn't support
// mipmaps for NPOT textures)
static int nearestPOT(int value) {
    return pow(2, ceil(log(value)/log(2)));
}

GLWidget::GLWidget(QWidget* parent) :
        QWidget(parent),
        mEGL((const EGLDispatch*)android_getEGLDispatch()),
        mGLES2((const GLESv2Dispatch*)android_getGLESv2Dispatch()),
        mEGLState(nullptr),
        mValid(false),
        mEnableAA(false) {
    setAutoFillBackground(false);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_PaintOnScreen, true);
    setAttribute(Qt::WA_NoSystemBackground, true);

    // QGLWidget always has a native window, and ours should behave
    // in a similar fashion as well. Not having these attributes set
    // causes issues with the window initialization.
    setAttribute(Qt::WA_DontCreateNativeAncestors, true);
    setAttribute(Qt::WA_NativeWindow, true);
}

void GLWidget::handleScreenChange(QScreen*) {
    // Destroy the context, forcing its re-creation on
    // next paint event.
    destroyContext();
    resizeGL(realPixelsWidth(), realPixelsHeight());
}

bool GLWidget::ensureInit() {
    // If an error occured when loading the EGL/GLESv2 libraries, return false.
    if (!mEGL || !mGLES2) {
        return false;
    }

    // If already initialized, return mValid to indicate if an error occured.
    if (mEGLState) {
        return mValid;
    }

    mEGLState = new EGLState();
    mValid = false;

    mEGLState->display = mEGL->eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (mEGLState->display == EGL_NO_DISPLAY) {
        qCWarning(emu, "Failed to get EGL display: EGL error %d",
                 mEGL->eglGetError());
        return false;
    }

    EGLint egl_maj, egl_min;
    EGLConfig egl_config;

    // Try to initialize EGL display.
    // Initializing an already-initialized display is OK.
    if (mEGL->eglInitialize(mEGLState->display, &egl_maj, &egl_min) ==
        EGL_FALSE) {
        qCWarning(emu, "Failed to initialize EGL display: EGL error %d",
                 mEGL->eglGetError());
        return false;
    }

    // Get an EGL config.
    const EGLint config_attribs[] = {EGL_SURFACE_TYPE,
                                     EGL_WINDOW_BIT,
                                     EGL_RENDERABLE_TYPE,
                                     EGL_OPENGL_ES2_BIT,
                                     EGL_RED_SIZE,
                                     8,
                                     EGL_GREEN_SIZE,
                                     8,
                                     EGL_BLUE_SIZE,
                                     8,
                                     EGL_DEPTH_SIZE,
                                     24,
                                     EGL_SAMPLES,
                                     0,  // No multisampling
                                     EGL_NONE};
    EGLint num_config;
    EGLBoolean choose_result = mEGL->eglChooseConfig(
            mEGLState->display, config_attribs, &egl_config, 1, &num_config);
    if (choose_result == EGL_FALSE || num_config < 1) {
        qCWarning(emu, "Failed to choose EGL config: EGL error %d",
                 mEGL->eglGetError());
        return false;
    }

    // Create a context.
    EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    AndroidVirtioGpuOps* ops = android_getVirtioGpuOps();
    // Share this context with the emulator framebuffer context by default.
    mEGLState->context = mEGL->eglCreateContext(
        mEGLState->display,
        egl_config,
        (EGLContext)ops->get_global_egl_context(),
        context_attribs);

    if (mEGLState->context == EGL_NO_CONTEXT) {
        qCWarning(emu, "Failed to create EGL context %d", mEGL->eglGetError());
    }

    // Finally, create a window surface associated with this widget.
    mEGLState->surface = mEGL->eglCreateWindowSurface(
            mEGLState->display, egl_config, (EGLNativeWindowType)(winId()),
            nullptr);
    if (mEGLState->surface == EGL_NO_SURFACE) {
        qCWarning(emu, "Failed to create an EGL surface %d", mEGL->eglGetError());
        return false;
    }


    makeContextCurrent();
    mCanvas.reset(new GLCanvas(
        nearestPOT(realPixelsWidth()),
        nearestPOT(realPixelsHeight()),
        mGLES2));
    mTextureDraw.reset(new TextureDraw(mGLES2));
    mValid = initGL();

    return mValid;
}

bool GLWidget::makeContextCurrent() {
    if (mEGLState) {
        return
            mEGL->eglMakeCurrent(mEGLState->display, mEGLState->surface,
                                 mEGLState->surface, mEGLState->context) == EGL_TRUE;

    } else {
        return false;
    }
}

void GLWidget::renderFrame() {
    if (!readyForRendering()) {
        return;
    }
    makeContextCurrent();

    // Render 3D scene to texture.
    if (mEnableAA) {
        mCanvas->bind();
        repaintGL();
        mCanvas->unbind();
    } else {
        mGLES2->glViewport(0, 0, realPixelsWidth(), realPixelsHeight());
        repaintGL();
    }

    if (mEnableAA) {
        mGLES2->glBindTexture(GL_TEXTURE_2D, mCanvas->texture());
        mGLES2->glGenerateMipmap(GL_TEXTURE_2D);
        mTextureDraw->draw(mCanvas->texture(),
                           realPixelsWidth(),
                           realPixelsHeight());
    }

    mEGL->eglSwapBuffers(mEGLState->display, mEGLState->surface);
}

void GLWidget::paintEvent(QPaintEvent*) {
    if (ensureInit()) {
        renderFrame();
    }
}

void GLWidget::showEvent(QShowEvent*) {
    // When the widget first becomes visible, we ask it to render a frame,
    // which will force the necessary initialization.
    // It is important to make sure that the widget is actually
    // visible on screen (at least for OS X) before doing any
    // initialization at all.
    // However, show events may be delivered when the widget
    // isn't visible yet, so we need an additional check.
    if (isVisible() && !visibleRegion().isNull() && mFirstShow) {
        mFirstShow = false;
        connect(window()->windowHandle(), SIGNAL(screenChanged(QScreen*)),
                this, SLOT(handleScreenChange(QScreen*)));
        update();
    }
}

void GLWidget::resizeEvent(QResizeEvent* e) {
    if (mEGLState) {
        // We should only call resizeGL and repaint if all the setup
        // has been done.
        // We should NOT attempt to initialize EGL state during a resize
        // event due to some subtleties on OS X. If we attempt to initialize
        // EGL state at the time the resize event is generated, the default
        // framebuffer will not be created.
        makeContextCurrent();
        // Re-create the framebuffer with new size.
        mCanvas.reset(new GLCanvas(
            nearestPOT(e->size().width() * devicePixelRatioF()),
            nearestPOT(e->size().height() * devicePixelRatioF()),
            mGLES2));
        resizeGL(e->size().width() * devicePixelRatioF(),
                 e->size().height() * devicePixelRatioF());
        update();
    }
}

GLWidget::~GLWidget() {
    destroyContext();
}

void GLWidget::destroyContext() {
    if (mGLES2 && mEGL && mEGLState) {
        // Destroy canvas and texturedraw state
        // within this context.
        makeContextCurrent();
        mCanvas.reset(nullptr);
        mTextureDraw.reset(nullptr);

        // Make sure the context isn't active before destroying it.
        mEGL->eglMakeCurrent(mEGLState->display, 0, 0, 0);

        // Reset EGL state and set mEGLState to null, which will force
        // re-initialization when attempting to render the next frame.
        mEGL->eglDestroySurface(mEGLState->display, mEGLState->surface);
        mEGL->eglDestroyContext(mEGLState->display, mEGLState->context);
        delete mEGLState;
        mEGLState = nullptr;
    }
}
