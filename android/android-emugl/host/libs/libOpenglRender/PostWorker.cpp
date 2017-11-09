#include "PostWorker.h"

#include "ColorBuffer.h"
#include "DispatchTables.h"
#include "FrameBuffer.h"
#include "RenderThreadInfo.h"
#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"

PostWorker::PostWorker(PostWorker::BindSubwinCallback&& cb) :
    mFb(FrameBuffer::getFB()),
    mBindSubwin(cb) {}

void PostWorker::post(ColorBuffer* cb) {
    // bind the subwindow eglSurface
    if (!m_initialized) {
        m_initialized = mBindSubwin();
    }

    float dpr = mFb->getDpr();
    int windowWidth = mFb->windowWidth();
    int windowHeight = mFb->windowHeight();
    float px = mFb->getPx();
    float py = mFb->getPy();
    int zRot = mFb->getZrot();

    GLuint tex = cb->scale();

    // find the x and y values at the origin when "fully scrolled"
    // multiply by 2 because the texture goes from -1 to 1, not 0 to 1
    // divide by device pixel ratio because windowing coordinates ignore
    // DPR,
    // but the framebuffer includes DPR
    float fx = 2.f * (m_viewportWidth / dpr - windowWidth) / (float)m_viewportWidth;
    float fy = 2.f * (m_viewportHeight / dpr - windowHeight) / (float)m_viewportHeight;

    // finally, compute translation values
    float dx = px * fx;
    float dy = py * fy;

    //
    // render the color buffer to the window
    //
    cb->post(tex, zRot, dx, dy);
    s_egl.eglSwapBuffers(mFb->getDisplay(), mFb->getWindowSurface());
}

// Called whenever the subwindow needs a refresh (FrameBuffer::setupSubWindow).
// This rebinds the subwindow context (to account for
// when the refresh is a display change, for instance)
// and resets the posting viewport.
void PostWorker::viewport(int width, int height) {
    // rebind the subwindow eglSurface unconditionally---
    // this could be from a display change
    m_initialized = mBindSubwin();

    float dpr = mFb->getDpr();
    m_viewportWidth = width * dpr;
    m_viewportHeight = height * dpr;
    s_gles2.glViewport(0, 0, m_viewportWidth, m_viewportHeight);
}

PostWorker::~PostWorker() {
    s_egl.eglMakeCurrent(
        mFb->getDisplay(),
        EGL_NO_SURFACE,
        EGL_NO_SURFACE,
        EGL_NO_CONTEXT);
}
