#pragma once

#include "android/base/Compiler.h"
#include "android/base/synchronization/Lock.h"

#include <EGL/egl.h>
#include <GLES/gl.h>
#include <GLES3/gl3.h>

#include "Hwc2.h"

#include <functional>
#include <vector>

class ColorBuffer;
class FrameBuffer;
struct RenderThreadInfo;

class PostWorker {
public:
    using BindSubwinCallback = std::function<bool(void)>;

    PostWorker(BindSubwinCallback&& cb);
    ~PostWorker();

    // post: posts the next color buffer.
    // Assumes framebuffer lock is held.
    void post(ColorBuffer* cb);

    // viewport: (re)initializes viewport dimensions.
    // Assumes framebuffer lock is held.
    // This is called whenever the subwindow needs a refresh (FrameBuffer::setupSubWindow).
    void viewport(int width, int height);

    // compose: compse the layers into final framebuffer
    void compose(ComposeDevice* p);

    // clear: blanks out emulator display when refreshing the subwindow
    // if there is no last posted color buffer to show yet.
    void clear();

private:
    void composeLayer(ComposeLayer* l);

private:
    EGLContext mContext;
    EGLSurface mSurf;
    RenderThreadInfo* mTLS;
    FrameBuffer* mFb;

    std::function<bool(void)> mBindSubwin;

    bool m_initialized = false;
    int m_viewportWidth = 0;
    int m_viewportHeight = 0;
    GLuint m_composeFbo = 0;
    DISALLOW_COPY_AND_ASSIGN(PostWorker);
};
