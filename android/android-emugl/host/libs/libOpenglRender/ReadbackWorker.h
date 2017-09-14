#pragma once

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <vector>

class RenderThreadInfo;
class FrameBuffer;

class ReadbackWorker {
public:
    // Constructor and setLastBuffer() on a single GL thread only.
    // Takes a vector of GL buffers sized appropriately for
    // the framebuffer to read back.
    ReadbackWorker(const std::vector<GLuint>& buffers);
    ~ReadbackWorker();

    // Function that copies the input buffer's first
    // |bytes| to |callback| that takes the resulting buffer data.
    void setLastBuffer(uint32_t index);

    void getPixels(void* out, uint32_t bytes);
private:
    EGLContext mContext;
    EGLSurface mSurf;
    RenderThreadInfo* mTLS;
    FrameBuffer* mFb;
    uint32_t mLastBufferIndex = 0;
    std::vector<GLuint> mBuffers = {};
};
