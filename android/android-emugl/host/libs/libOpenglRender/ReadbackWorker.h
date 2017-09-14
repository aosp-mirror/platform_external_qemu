#pragma once

#include "android/base/synchronization/Lock.h"

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <functional>

struct RenderThreadInfo;

class ReadbackWorker {
public:
    // Constructor and readback() on a single GL thread only.
    ReadbackWorker(GLuint inputBuffer, uint32_t sz);

    // Function that copies the input buffer's first
    // |bytes| to |dst|. Assumes |dst| is allocated correctly.
    void readback(GLuint bufferId, uint32_t bytes, void* dst, std::function<void()> callback);

    // For protecting access to mBuffer
    android::base::Lock& lock() {
        return mLock;
    }

private:
    void resizeBufferForHost(uint32_t sz);

    android::base::Lock mLock;

    uint32_t mContext;
    uint32_t mSurf;
    GLuint mBuffer;
    GLuint mBufferForHost;
    uint32_t mBufferForHostSz = 0;
    RenderThreadInfo* mTLS;
};
