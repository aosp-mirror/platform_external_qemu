#pragma once

#include <EGL/egl.h>
#include <GLES3/gl3.h>

class RenderThreadInfo;
class FrameBuffer;

class ReadbackWorker {
public:
    // Constructor and readback() on a single GL thread only.
    ReadbackWorker();
    ~ReadbackWorker();

    // Function that copies the input buffer's first
    // |bytes| to |callback| that takes the resulting buffer data.
    void readback(GLuint bufferId, uint32_t bytes);
private:
    EGLContext mContext;
    EGLSurface mSurf;
    RenderThreadInfo* mTLS;
    FrameBuffer* mFb;
};
