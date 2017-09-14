
#include "ReadbackWorker.h"

#include "ColorBuffer.h"
#include "DispatchTables.h"
#include "FrameBuffer.h"
#include "RenderThreadInfo.h"
#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"

#include "android/base/Stopwatch.h"

ReadbackWorker::ReadbackWorker(uint32_t width, uint32_t height) :
    mFb(FrameBuffer::getFB()),
    mBufferSize(4 * width * height),
    mBuffers(3 /* mailbox */, 0) {
}

void ReadbackWorker::initGL() {
    mFb->createAndBindTrivialSharedContext(&mContext, &mSurf);
    s_gles2.glGenBuffers(mBuffers.size(), &mBuffers[0]);
    for (uint32_t i = 0; i < mBuffers.size(); i++) {
        s_gles2.glBindBuffer(GL_PIXEL_PACK_BUFFER, mBuffers[i]);
        s_gles2.glBufferData(GL_PIXEL_PACK_BUFFER, mBufferSize, 0 /* init, with no data */,
                             GL_STREAM_READ);
    }
    s_gles2.glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

ReadbackWorker::~ReadbackWorker() {
    mFb->unbindAndDestroyTrivialSharedContext(mContext, mSurf);
}

void ReadbackWorker::doNextReadback(ColorBuffer* cb, void* fbImage) {
    // Mailbox-style triple buffering setup:
    // We want to avoid glReadPixels while in the middle of doing
    // memcpy to the consumer, but also want to avoid latency while
    // that is going on.
    //
    // There are 3 buffer ids, A, B, and C.
    // If we are not in the middle of copying out a frame,
    // set glReadPixels to write to buffer A and copy from buffer B in
    // alternation, so the consumer always gets the latest frame
    // +1 frame of lag in order to not cause blocking on
    // glReadPixels / glMapBufferRange.
    // If we are in the middle of copying out a frame, reset A and B to
    // not be the buffer being copied out, and continue glReadPixels to
    // buffer A and B as before.
    bool isCopying = mIsCopying.load(std::memory_order_relaxed);
    if (isCopying) {
        switch (mMapCopyIndex) {
        case 0:
            mReadPixelsIndex = 2;
            mReadPixels2Index = 1;
            break;
        case 1:
            mReadPixelsIndex = 0;
            mReadPixels2Index = 2;
            break;
        case 2:
            mReadPixelsIndex = 0;
            mReadPixels2Index = 1;
            break;
        }
    } else {
        mReadPixelsIndex = 0;
        mReadPixels2Index = 1;
    }

    // Double buffering on buffer A / B part
    uint32_t readAt;
    if (m_readbackCount % 2 == 0) {
        readAt = mReadPixelsIndex;
        if (!isCopying) {
            mMapCopyIndex = mReadPixels2Index;
        }
    } else {
        readAt = mReadPixels2Index;
        if (!isCopying) {
            mMapCopyIndex = mReadPixelsIndex;
        }
    }

    cb->readbackAsync(mBuffers[readAt]);
    mFb->doPostCallback(fbImage);
    m_readbackCount++;
}

void ReadbackWorker::getPixels(void* buf, uint32_t bytes) {
    mIsCopying.store(true, std::memory_order_relaxed);

    GLuint buffer = mBuffers[mMapCopyIndex];
    s_gles2.glBindBuffer(GL_COPY_READ_BUFFER, buffer);
    void* pixels = s_gles2.glMapBufferRange(GL_COPY_READ_BUFFER, 0, bytes, GL_MAP_READ_BIT);
    memcpy(buf, pixels, bytes);
    s_gles2.glUnmapBuffer(GL_COPY_READ_BUFFER);

    mIsCopying.store(false, std::memory_order_relaxed);
}
