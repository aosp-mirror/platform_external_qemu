#include "android/skin/qt/gl-widget.h"
#include "GLES2/gl2.h"
#include <QResizeEvent>
#include <QtGlobal>

// Note that this header must come after ALL Qt headers.
// Including any Qt headers after it will cause compilation
// to fail spectacularly. This is because EGL indirectly
// includes X11 headers which define stuff that conflicts
// with Qt's own macros. It is a known issue.
#include "EGLDispatch.h"

gles2_server_context_t gles2;

struct EGLState {
    EGLDisplay Display;
    EGLConfig Config;
    EGLContext Context;
    EGLSurface Surface;
};

GLWidget::GLWidget(QWidget* parent) :
    QWidget(parent),
    mEGLState(nullptr),
    mGLES2(&gles2) {
    if (mGLES2->glClear == nullptr) {
        init_gles2_dispatch(mGLES2);
    }
    setAutoFillBackground(false);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_PaintOnScreen, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
}

bool GLWidget::setupContextAndSurface() {
    if (mEGLState == nullptr) {
        mEGLState = new EGLState();
    }

    if (s_egl.eglGetDisplay == nullptr) {
        // Initialize the EGL function dispatch table if it hasn't been
        // initialized yet.
        init_egl_dispatch();
    }

    mEGLState->Display = s_egl.eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (mEGLState->Display == EGL_NO_DISPLAY) {
        qWarning("Failed to get EGL display");
        return false;
    }

    EGLint egl_maj, egl_min;

    // Try to initialize EGL display.
    // Initializing an already-initialized display is OK.
    if (s_egl.eglInitialize(mEGLState->Display, &egl_maj, &egl_min) == EGL_FALSE) {
        qWarning("Failed to initialize EGL display");
        return false;
    }

    // Get an EGL config.
    EGLint config_attribs[] =
        {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_DEPTH_SIZE, 24,
            EGL_NONE
        };
    EGLint num_config;
    if (s_egl.eglChooseConfig(
                mEGLState->Display,
                config_attribs,
                &(mEGLState->Config),
                1,
                &num_config) == EGL_FALSE) {
        qWarning("Failed to choose EGL config");
        return false;
    }

    // Create a context.
    EGLint context_attribs[] =
        {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
        };
    mEGLState->Context = s_egl.eglCreateContext(
            mEGLState->Display,
            mEGLState->Config,
            EGL_NO_CONTEXT,
            context_attribs);
    if (mEGLState->Context == EGL_NO_CONTEXT) {
        qWarning("Failed to create EGL context");
    }

    // Finally, create a window surface associated with this widget.
    mEGLState->Surface = s_egl.eglCreateWindowSurface(
            mEGLState->Display,
            mEGLState->Config,
            (EGLNativeWindowType)(winId()),
            nullptr);
    if (mEGLState->Surface == EGL_NO_SURFACE) {
        qWarning("Failed to create an EGL surface");
        return false;
    }

    return true;

}

void GLWidget::makeContextCurrent() {
    s_egl.eglMakeCurrent(
            mEGLState->Display,
            mEGLState->Surface,
            mEGLState->Surface,
            mEGLState->Context);
}

void GLWidget::renderFrame() {
    if (mEGLState == nullptr) {
        // Make sure the initialization is done.
        // Calling setupContextAndSurface from the constructor will not
        // work (at least on some platforms).
        setupContextAndSurface();
        makeContextCurrent();
        initGL();
    } else {
        makeContextCurrent();
    }
    repaintGL();
    s_egl.eglSwapBuffers(mEGLState->Display, mEGLState->Surface);
}

void GLWidget::paintEvent(QPaintEvent*) {
    renderFrame();
}

void GLWidget::resizeEvent(QResizeEvent* e) {
    if (mEGLState) {
        // Resize event might arrive before the first paint event.
        // We should only call resizeGL and repaint if all the setup
        // has been done.
        makeContextCurrent();
        resizeGL(e->size().width(), e->size().height());
        renderFrame();
    }
}


GLWidget::~GLWidget() {
    s_egl.eglDestroyContext(mEGLState->Display, mEGLState->Context);
    s_egl.eglDestroySurface(mEGLState->Display, mEGLState->Surface);
    delete mEGLState;
}

