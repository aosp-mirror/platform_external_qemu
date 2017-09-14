
#include "ReadbackWorker.h"

#include "DispatchTables.h"
#include "FrameBuffer.h"
#include "RenderThreadInfo.h"
#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"

#include "android/base/Stopwatch.h"

ReadbackWorker::ReadbackWorker(const std::vector<GLuint>& buffers) :
    mFb(FrameBuffer::getFB()),
    mBuffers(buffers) {
}

void ReadbackWorker::initGL() {
    mFb->createAndBindTrivialSharedContext(&mContext, &mSurf);
}

ReadbackWorker::~ReadbackWorker() {
    mFb->unbindAndDestroyTrivialSharedContext(mContext, mSurf);
}

void ReadbackWorker::setLastBuffer(uint32_t index) {
    android::base::AutoLock lock(mLock);
    mLastBufferIndex = index;
}

void ReadbackWorker::getPixels(void* buf, uint32_t bytes) {
    android::base::AutoLock lock(mLock);
    uint32_t currBufIndex = mLastBufferIndex;
    uint32_t untouchedIndex = (currBufIndex + mBuffers.size() - 1) % mBuffers.size();
    lock.unlock();

    GLuint buffer = mBuffers[untouchedIndex];
    s_gles2.glBindBuffer(GL_COPY_READ_BUFFER, buffer);
    void* pixels = s_gles2.glMapBufferRange(GL_COPY_READ_BUFFER, 0, bytes, GL_MAP_READ_BIT);
    memcpy(buf, pixels, bytes);
    s_gles2.glUnmapBuffer(GL_COPY_READ_BUFFER);
}

