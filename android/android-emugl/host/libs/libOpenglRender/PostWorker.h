#pragma once

#include "android/base/Compiler.h"
#include "android/base/synchronization/Lock.h"

#include <EGL/egl.h>

#include <vector>

class ColorBuffer;
class FrameBuffer;
class RenderThreadInfo;

class PostWorker {
public:
    PostWorker();
    ~PostWorker();

    void post(ColorBuffer* cb);
    void viewport(int width, int height);

private:
    EGLContext mContext;
    EGLSurface mSurf;
    RenderThreadInfo* mTLS;
    FrameBuffer* mFb;

    bool m_initialized = false;

    DISALLOW_COPY_AND_ASSIGN(PostWorker);
};
