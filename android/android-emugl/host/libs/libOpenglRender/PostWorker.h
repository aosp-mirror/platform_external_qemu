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

private:
    EGLContext mContext;
    EGLSurface mSurf;
    RenderThreadInfo* mTLS;
    FrameBuffer* mFb;

    android::base::Lock mLock;

    DISALLOW_COPY_AND_ASSIGN(PostWorker);
};
