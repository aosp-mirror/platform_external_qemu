
#include "ReadbackWorker.h"

#include <string.h>                           // for memcpy

#include "ColorBuffer.h"                      // for ColorBuffer
#include "DispatchTables.h"                   // for s_gles2
#include "FbConfig.h"                         // for FbConfig, FbConfigList
#include "FrameBuffer.h"                      // for FrameBuffer
#include "OpenGLESDispatch/EGLDispatch.h"     // for EGLDispatch, s_egl
#include "OpenGLESDispatch/GLESv2Dispatch.h"  // for GLESv2Dispatch
#include "emugl/common/misc.h"                // for getGlesVersion

ReadbackWorker::ReadbackWorker(uint32_t width,
                               uint32_t height,
                               uint32_t displayId)
    : mFb(FrameBuffer::getFB()),
      mBufferSize(4 * width * height /* RGBA8 (4 bpp) */),
      mBuffers(4 /* mailbox */,
               0),  // Note, last index is used for duplicating buffer on flush
      mDisplayId(displayId) {}

void ReadbackWorker::initGL() {
    mFb->createAndBindTrivialSharedContext(&mContext, &mSurf);
    mFb->createAndBindTrivialSharedContext(&mFlushContext, &mFlushSurf);
    s_gles2.glGenBuffers(mBuffers.size(), &mBuffers[0]);
    for (auto buffer : mBuffers) {
        s_gles2.glBindBuffer(GL_PIXEL_PACK_BUFFER, buffer);
        s_gles2.glBufferData(GL_PIXEL_PACK_BUFFER, mBufferSize,
                             0 /* init, with no data */, GL_STREAM_READ);
    }

    s_gles2.glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

ReadbackWorker::~ReadbackWorker() {
    s_gles2.glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    s_gles2.glBindBuffer(GL_COPY_READ_BUFFER, 0);
    s_gles2.glDeleteBuffers(mBuffers.size(), &mBuffers[0]);
    mFb->unbindAndDestroyTrivialSharedContext(mContext, mSurf);
    mFb->unbindAndDestroyTrivialSharedContext(mFlushContext, mFlushSurf);
}

void ReadbackWorker::doNextReadback(ColorBuffer* cb,
                                    void* fbImage,
                                    bool repaint,
                                    bool readbackBgra) {
    // if |repaint|, make sure that the current frame is immediately sent down
    // the pipeline and made available to the consumer by priming async
    // readback; doing 4 consecutive reads in a row, which should be enough to
    // fill the 3 buffers in the triple buffering setup and on the 4th, trigger
    // a post callback.
    int numIter = repaint ? 4 : 1;

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
    //
    // The resulting invariants are:
    // - glReadPixels is called on a different buffer every time
    //   so we avoid introducing sync points there.
    // - At no time are we mapping/copying a buffer and also doing
    //   glReadPixels on it (avoid simultaneous map + glReadPixels)
    // - glReadPixels and then immediately map/copy the same buffer
    //   doesn't happen either (avoid sync point in glMapBufferRange)
    for (int i = 0; i < numIter; i++) {
        android::base::AutoLock lock(mLock);
        if (mIsCopying) {
            switch (mMapCopyIndex) {
                // To keep double buffering effect on
                // glReadPixels, need to keep even/oddness of
                // mReadPixelsIndexEven and mReadPixelsIndexOdd.
                case 0:
                    mReadPixelsIndexEven = 2;
                    mReadPixelsIndexOdd = 1;
                    break;
                case 1:
                    mReadPixelsIndexEven = 0;
                    mReadPixelsIndexOdd = 2;
                    break;
                case 2:
                    mReadPixelsIndexEven = 0;
                    mReadPixelsIndexOdd = 1;
                    break;
            }
        } else {
            mReadPixelsIndexEven = 0;
            mReadPixelsIndexOdd = 1;
            mMapCopyIndex = mPrevReadPixelsIndex;
        }

        // Double buffering on buffer A / B part
        uint32_t readAt;
        if (m_readbackCount % 2 == 0) {
            readAt = mReadPixelsIndexEven;
        } else {
            readAt = mReadPixelsIndexOdd;
        }
        m_readbackCount++;
        mPrevReadPixelsIndex = readAt;

        cb->readbackAsync(mBuffers[readAt], readbackBgra);

        // It's possible to post callback before any of the async readbacks
        // have written any data yet, which results in a black frame.  Safer
        // option to avoid this glitch is to wait until all 3 potential
        // buffers in our triple buffering setup have had chances to readback.
        lock.unlock();
        if (m_readbackCount > 3) {
            mFb->doPostCallback(fbImage, mDisplayId);
        }
    }
}

void ReadbackWorker::flushPipeline() {
    android::base::AutoLock lock(mLock);
    if (mIsCopying) {
        // No need to make the last frame available,
        // we are currently being read.
        return;
    }

    auto src = mBuffers[mPrevReadPixelsIndex];
    auto dst = mBuffers.back();

    // This is not called from a renderthread, so let's activate
    // the context.
    s_egl.eglMakeCurrent(mFb->getDisplay(), mFlushSurf, mFlushSurf, mFlushContext);

    // We now copy the last frame into slot 4, where no other thread
    // ever writes.
    s_gles2.glBindBuffer(GL_COPY_READ_BUFFER, src);
    s_gles2.glBindBuffer(GL_COPY_WRITE_BUFFER, dst);
    s_gles2.glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0,
                                mBufferSize);
    s_egl.eglMakeCurrent(mFb->getDisplay(), EGL_NO_SURFACE, EGL_NO_SURFACE,
                         EGL_NO_CONTEXT);

    mMapCopyIndex = mBuffers.size() - 1;
    lock.unlock();
    mFb->doPostCallback(nullptr, mDisplayId);
}

void ReadbackWorker::getPixels(void* buf, uint32_t bytes) {
    android::base::AutoLock lock(mLock);
    mIsCopying = true;
    lock.unlock();

    GLuint buffer = mBuffers[mMapCopyIndex];
    s_gles2.glBindBuffer(GL_COPY_READ_BUFFER, buffer);
    void* pixels = s_gles2.glMapBufferRange(GL_COPY_READ_BUFFER, 0, bytes,
                                            GL_MAP_READ_BIT);
    memcpy(buf, pixels, bytes);
    s_gles2.glUnmapBuffer(GL_COPY_READ_BUFFER);

    lock.lock();
    mIsCopying = false;
    lock.unlock();
}
