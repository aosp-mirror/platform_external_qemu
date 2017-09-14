
#include "ReadbackWorker.h"

#include "DispatchTables.h"
#include "FrameBuffer.h"
#include "RenderThreadInfo.h"
#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"

#include "android/base/Stopwatch.h"

using android::base::FunctionView;

ReadbackWorker::ReadbackWorker() {
    mTLS = new RenderThreadInfo();

    FrameBuffer::getFB()->createTrivialSharedContext(&mContext, &mSurf);
    FrameBuffer::getFB()->bindContext(mContext, mSurf, mSurf);
}

void ReadbackWorker::readback(GLuint bufferId, uint32_t bytes,
                              FunctionView<void(void*)> callback) {
    s_gles2.glBindBuffer(GL_COPY_READ_BUFFER, bufferId);
    void* toCopy = s_gles2.glMapBufferRange(GL_COPY_READ_BUFFER, 0, bytes, GL_MAP_READ_BIT);
    callback(toCopy);
    s_gles2.glUnmapBuffer(GL_COPY_READ_BUFFER);
}
