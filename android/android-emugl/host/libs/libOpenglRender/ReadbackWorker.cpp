
#include "ReadbackWorker.h"

#include "DispatchTables.h"
#include "FrameBuffer.h"
#include "RenderThreadInfo.h"
#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"

#include "android/base/Stopwatch.h"

using android::base::AutoLock;

ReadbackWorker::ReadbackWorker(GLuint inputBuffer, uint32_t sz) :
    mBuffer(inputBuffer), mBufferForHostSz(sz)  {

    mTLS = new RenderThreadInfo();

    FrameBuffer::getFB()->createTrivialSharedContext(&mContext, &mSurf);
    FrameBuffer::getFB()->bindContext(mContext, mSurf, mSurf);

    s_gles2.glBindBuffer(GL_COPY_READ_BUFFER, mBuffer);
    s_gles2.glGenBuffers(1, &mBufferForHost);
    s_gles2.glBindBuffer(GL_COPY_WRITE_BUFFER, mBufferForHost);

    GLint bufsize;
    s_gles2.glGetBufferParameteriv(GL_COPY_READ_BUFFER, GL_BUFFER_SIZE, &bufsize);
    resizeBufferForHost(mBufferForHostSz);
}

void ReadbackWorker::resizeBufferForHost(uint32_t sz) {
    mBufferForHostSz = sz;
    s_gles2.glBufferData(GL_COPY_WRITE_BUFFER, sz, nullptr, GL_DYNAMIC_DRAW);
}

using android::base::Stopwatch;
void ReadbackWorker::readback(GLuint bufferId, uint32_t bytes, void* dst, std::function<void()> callback) {
    s_gles2.glBindBuffer(GL_COPY_READ_BUFFER, bufferId);
    void* toCopy = s_gles2.glMapBufferRange(GL_COPY_READ_BUFFER, 0, bytes, GL_MAP_READ_BIT);
    memcpy(dst, toCopy, bytes);
    s_gles2.glUnmapBuffer(GL_COPY_READ_BUFFER);
    callback();
}
