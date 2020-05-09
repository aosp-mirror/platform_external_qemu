#pragma once

#include <EGL/egl.h>                            // for EGLContext, EGLSurface
#include <GLES3/gl3.h>                          // for GLuint
#include <map>                                  // for map
#include <stdint.h>                             // for uint32_t
#include <vector>                               // for vector

#include "android/base/Compiler.h"              // for DISALLOW_COPY_AND_ASSIGN
#include "android/base/synchronization/Lock.h"  // for Lock

class ColorBuffer;
class FrameBuffer;
struct RenderThreadInfo;

// This class implements async readback of emugl ColorBuffers.
// It is meant to run on both the emugl framebuffer posting thread
// and a separate GL thread, with two main points of interaction:
class ReadbackWorker {
public:
    ReadbackWorker() = default;
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
    // |readbackBgra|: Whether to force the readback format as GL_BGRA_EXT,
    // so that we get (depending on driver quality, heh) a gpu conversion of the
    // readback image that is suitable for webrtc, which expects formats like that.
    void doNextReadback(uint32_t displayId, ColorBuffer* cb, void* fbImage, bool repaint, bool readbackBgra);

    // getPixels(): Run this on a separate GL thread. This retrieves the
    // latest framebuffer that has been posted and read with doNextReadback.
    // This is meant for apps like video encoding to use as input; they will
    // need to do synchronized communication with the thread ReadbackWorker
    // is running on.
    void getPixels(uint32_t displayId, void* out, uint32_t bytes);

    // Duplicates the last frame and generates a post events if
    // there are no read events active.
    // This is usually called when there was no doNextReadback activity
    // for a few ms, to guarantee that end users see the final frame.
    void flushPipeline(uint32_t displayId);

    void setRecordDisplay(uint32_t displayId, uint32_t w, uint32_t h, bool add);

    class recordDisplay {
    public:
        recordDisplay() = default;
        recordDisplay(uint32_t displayId, uint32_t w, uint32_t h);
    public:
        uint32_t mReadPixelsIndexEven = 0;
        uint32_t mReadPixelsIndexOdd = 1;
        uint32_t mPrevReadPixelsIndex = 1;
        uint32_t mMapCopyIndex = 0;
        bool mIsCopying = false;
        uint32_t mBufferSize = 0;
        std::vector<GLuint> mBuffers = {};
        uint32_t m_readbackCount = 0;
        uint32_t mDisplayId = 0;
    };

private:
    EGLContext mContext;
    EGLContext mFlushContext;
    EGLSurface mSurf;
    EGLSurface mFlushSurf;

    FrameBuffer* mFb;
    android::base::Lock mLock;

    std::map<uint32_t, recordDisplay> mRecordDisplays;

    DISALLOW_COPY_AND_ASSIGN(ReadbackWorker);
};
