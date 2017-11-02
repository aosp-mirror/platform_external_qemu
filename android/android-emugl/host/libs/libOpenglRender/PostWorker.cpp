#include "PostWorker.h"

#include "ColorBuffer.h"
#include "DispatchTables.h"
#include "FrameBuffer.h"
#include "RenderThreadInfo.h"
#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"

PostWorker::PostWorker() :
    mFb(FrameBuffer::getFB()) {
}

void PostWorker::post(ColorBuffer* cb) {
    // bind the subwindow eglSurface
    if (!m_initialized) {
        m_initialized = mFb->bindSubwin_locked();
    }

    float dpr = mFb->getDpr();
    int windowWidth = mFb->windowWidth();
    int windowHeight = mFb->windowHeight();
    float px = mFb->getPx();
    float py = mFb->getPy();
    int zRot = mFb->getZrot();

    GLuint tex = cb->scale();

    // get the viewport
    GLint vp[4];
    s_gles2.glGetIntegerv(GL_VIEWPORT, vp);

    // divide by device pixel ratio because windowing coordinates ignore
    // DPR,
    // but the framebuffer includes DPR
    vp[2] = vp[2] / dpr;
    vp[3] = vp[3] / dpr;

    // find the x and y values at the origin when "fully scrolled"
    // multiply by 2 because the texture goes from -1 to 1, not 0 to 1
    float fx = 2.f * (vp[2] - windowWidth) / (float)vp[2];
    float fy = 2.f * (vp[3] - windowHeight) / (float)vp[3];

    // finally, compute translation values
    float dx = px * fx;
    float dy = py * fy;

    //
    // render the color buffer to the window
    //
    cb->post(tex, zRot, dx, dy);
    s_egl.eglSwapBuffers(mFb->getDisplay(), mFb->getWindowSurface());
}

void PostWorker::viewport(int width, int height) {
    // bind the subwindow eglSurface
    if (!m_initialized) {
        m_initialized = mFb->bindSubwin_locked();
    }

    float dpr = mFb->getDpr();
    s_gles2.glViewport(0, 0, width * dpr, height * dpr);
}

PostWorker::~PostWorker() { }
