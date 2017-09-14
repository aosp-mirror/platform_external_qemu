
#include "ReadbackWorker.h"

#include "DispatchTables.h"
#include "FrameBuffer.h"
#include "RenderThreadInfo.h"
#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"

#include "android/base/Stopwatch.h"

ReadbackWorker::ReadbackWorker() {
    mFb = FrameBuffer::getFB();
    mFb->createAndBindTrivialSharedContext(&mContext, &mSurf);
}

ReadbackWorker::~ReadbackWorker() {
    mFb->unbindAndDestroyTrivialSharedContext(mContext, mSurf);
}

void ReadbackWorker::readback(GLuint bufferId, uint32_t bytes) {
    s_gles2.glBindBuffer(GL_COPY_READ_BUFFER, bufferId);
    void* pixels = s_gles2.glMapBufferRange(GL_COPY_READ_BUFFER, 0, bytes, GL_MAP_READ_BIT);
    mFb->doPostCallback(pixels);
    s_gles2.glUnmapBuffer(GL_COPY_READ_BUFFER);
}
