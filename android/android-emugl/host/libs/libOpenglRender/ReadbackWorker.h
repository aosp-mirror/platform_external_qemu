#pragma once

#include "android/base/synchronization/Lock.h"

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <memory>
#include <vector>

class ColorBuffer;
class FrameBuffer;
class RenderThreadInfo;

class ReadbackWorker {
public:
    // Constructor and setLastBuffer() on a single GL thread only.
    ReadbackWorker(uint32_t width, uint32_t height);
    ~ReadbackWorker();

    // GL initialization (must be on one thread)
    void initGL();

    void doNextReadback(ColorBuffer* cb, void* fbImage);

    // Function that copies the input buffer's first
    // |bytes| to |callback| that takes the resulting buffer data.
    void setLastBuffer(uint32_t index);

    void getPixels(void* out, uint32_t bytes);
private:
    EGLContext mContext;
    EGLSurface mSurf;
    RenderThreadInfo* mTLS;
    FrameBuffer* mFb;
    
    android::base::Lock mLock;
    uint32_t mReadPixelsIndex = 0;
    uint32_t mReadPixels2Index = 1;
    uint32_t mMapCopyIndex = 1;
    std::atomic<bool> mIsCopying { false };

    std::vector<GLuint> mBuffers = {};
    uint32_t mBufferSize = 0;
    uint32_t m_readbackCount = 0;
};
