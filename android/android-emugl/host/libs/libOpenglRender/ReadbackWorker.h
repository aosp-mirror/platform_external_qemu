#pragma once

#include "android/base/Compiler.h"
#include "android/base/synchronization/Lock.h"

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <vector>

class ColorBuffer;
class FrameBuffer;
struct RenderThreadInfo;

// This class implements async readback of emugl ColorBuffers.
// It is meant to run on both the emugl framebuffer posting thread
// and a separate GL thread, with two main points of interaction:
class ReadbackWorker {
public:
    ReadbackWorker(uint32_t width, uint32_t height);
    ~ReadbackWorker();

    // GL initialization (must be on the thread that
    // will run getPixels)
    void initGL();

    // doNextReadback(): Call this from the emugl FrameBuffer::post thread
    // or similar rendering thread.
    // This will trigger an async glReadPixels of the current framebuffer.
    // The post callback of Framebuffer will also be triggered, but
    // in async mode it should do minimal work that involves |fbImage|.
    // |repaint|: flag to prime async readback with multiple iterations
    // so that the consumer of readback doesn't lag behind.
    void doNextReadback(ColorBuffer* cb, void* fbImage, bool repaint);

    // getPixels(): Run this on a separate GL thread. This retrieves the
    // latest framebuffer that has been posted and read with doNextReadback.
    // This is meant for apps like video encoding to use as input; they will
    // need to do synchronized communication with the thread ReadbackWorker
    // is running on.
    void getPixels(void* out, uint32_t bytes);
private:
    EGLContext mContext;
    EGLSurface mSurf;
    RenderThreadInfo* mTLS;
    FrameBuffer* mFb;

    android::base::Lock mLock;
    uint32_t mReadPixelsIndexEven = 0;
    uint32_t mReadPixelsIndexOdd = 1;
    uint32_t mPrevReadPixelsIndex = 1;
    uint32_t mMapCopyIndex = 0;
    bool mIsCopying = false;

    uint32_t mBufferSize = 0;
    std::vector<GLuint> mBuffers = {};
    uint32_t m_readbackCount = 0;

    DISALLOW_COPY_AND_ASSIGN(ReadbackWorker);
};
