#pragma once

#include "android/base/FunctionView.h"

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <functional>

struct RenderThreadInfo;

class ReadbackWorker {
public:
    // Constructor and readback() on a single GL thread only.
    ReadbackWorker();

    // Function that copies the input buffer's first
    // |bytes| to |callback| that takes the resulting buffer data.
    void readback(GLuint bufferId, uint32_t bytes,
                  android::base::FunctionView<void(void*)> callback);

private:
    uint32_t mContext;
    uint32_t mSurf;
    RenderThreadInfo* mTLS;
};
