
#include "ReadbackWorker.h"

#include <string.h>                           // for memcpy

#include "ColorBuffer.h"                      // for ColorBuffer
#include "DispatchTables.h"                   // for s_gles2
#include "FbConfig.h"                         // for FbConfig, FbConfigList
#include "FrameBuffer.h"                      // for FrameBuffer
#include "OpenGLESDispatch/EGLDispatch.h"     // for EGLDispatch, s_egl
#include "OpenGLESDispatch/GLESv2Dispatch.h"  // for GLESv2Dispatch
#include "emugl/common/misc.h"                // for getGlesVersion

ReadbackWorker::recordDisplay::recordDisplay(uint32_t displayId, uint32_t w, uint32_t h)
    : mBufferSize(4 * w * h /* RGBA8 (4 bpp) */),
      mBuffers(4 /* mailbox */,
               0),  // Note, last index is used for duplicating buffer on flush
      mDisplayId(displayId) {}

void ReadbackWorker::initGL() {
    mFb = FrameBuffer::getFB();
    mFb->createAndBindTrivialSharedContext(&mContext, &mSurf);
    mFb->createAndBindTrivialSharedContext(&mFlushContext, &mFlushSurf);
}

ReadbackWorker::~ReadbackWorker() {
    s_gles2.glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    s_gles2.glBindBuffer(GL_COPY_READ_BUFFER, 0);
    for (auto& r : mRecordDisplays) {
        s_gles2.glDeleteBuffers(r.second.mBuffers.size(), &r.second.mBuffers[0]);
    }
    mFb->unbindAndDestroyTrivialSharedContext(mContext, mSurf);
    mFb->unbindAndDestroyTrivialSharedContext(mFlushContext, mFlushSurf);
}

void ReadbackWorker::setRecordDisplay(uint32_t displayId, uint32_t w, uint32_t h, bool add) {
    android::base::AutoLock lock(mLock);
    if (add) {
        mRecordDisplays.emplace(displayId, recordDisplay(displayId, w, h));
        recordDisplay& r = mRecordDisplays[displayId];
        s_gles2.glGenBuffers(r.mBuffers.size(), &r.mBuffers[0]);
        for (auto buffer : r.mBuffers) {
            s_gles2.glBindBuffer(GL_PIXEL_PACK_BUFFER, buffer);
            s_gles2.glBufferData(GL_PIXEL_PACK_BUFFER, r.mBufferSize,
                             0 /* init, with no data */, GL_STREAM_READ);
        }
        s_gles2.glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    } else {
        recordDisplay& r = mRecordDisplays[displayId];
        s_gles2.glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        s_gles2.glBindBuffer(GL_COPY_READ_BUFFER, 0);
        s_gles2.glDeleteBuffers(r.mBuffers.size(), &r.mBuffers[0]);
        mRecordDisplays.erase(displayId);
    }
}

void ReadbackWorker::doNextReadback(uint32_t displayId,
                                    ColorBuffer* cb,
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
        recordDisplay& r = mRecordDisplays[displayId];
        if (r.mIsCopying) {
            switch (r.mMapCopyIndex) {
                // To keep double buffering effect on
                // glReadPixels, need to keep even/oddness of
                // mReadPixelsIndexEven and mReadPixelsIndexOdd.
                case 0:
                    r.mReadPixelsIndexEven = 2;
                    r.mReadPixelsIndexOdd = 1;
                    break;
                case 1:
                    r.mReadPixelsIndexEven = 0;
                    r.mReadPixelsIndexOdd = 2;
                    break;
                case 2:
                    r.mReadPixelsIndexEven = 0;
                    r.mReadPixelsIndexOdd = 1;
                    break;
            }
        } else {
            r.mReadPixelsIndexEven = 0;
            r.mReadPixelsIndexOdd = 1;
            r.mMapCopyIndex = r.mPrevReadPixelsIndex;
        }

        // Double buffering on buffer A / B part
        uint32_t readAt;
        if (r.m_readbackCount % 2 == 0) {
            readAt = r.mReadPixelsIndexEven;
        } else {
            readAt = r.mReadPixelsIndexOdd;
        }
        r.m_readbackCount++;
        r.mPrevReadPixelsIndex = readAt;

        cb->readbackAsync(r.mBuffers[readAt], readbackBgra);

        // It's possible to post callback before any of the async readbacks
        // have written any data yet, which results in a black frame.  Safer
        // option to avoid this glitch is to wait until all 3 potential
        // buffers in our triple buffering setup have had chances to readback.
        lock.unlock();
        if (r.m_readbackCount > 3) {
            mFb->doPostCallback(fbImage, r.mDisplayId);
        }
    }
}

void ReadbackWorker::flushPipeline(uint32_t displayId) {
    android::base::AutoLock lock(mLock);
    recordDisplay& r = mRecordDisplays[displayId];
    if (r.mIsCopying) {
        // No need to make the last frame available,
        // we are currently being read.
        return;
    }

    auto src = r.mBuffers[r.mPrevReadPixelsIndex];
    auto dst = r.mBuffers.back();

    // This is not called from a renderthread, so let's activate
    // the context.
    s_egl.eglMakeCurrent(mFb->getDisplay(), mFlushSurf, mFlushSurf, mFlushContext);

    // We now copy the last frame into slot 4, where no other thread
    // ever writes.
    s_gles2.glBindBuffer(GL_COPY_READ_BUFFER, src);
    s_gles2.glBindBuffer(GL_COPY_WRITE_BUFFER, dst);
    s_gles2.glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0,
                                r.mBufferSize);
    s_egl.eglMakeCurrent(mFb->getDisplay(), EGL_NO_SURFACE, EGL_NO_SURFACE,
                         EGL_NO_CONTEXT);

    r.mMapCopyIndex = r.mBuffers.size() - 1;
    lock.unlock();
    mFb->doPostCallback(nullptr, r.mDisplayId);
}

void ReadbackWorker::getPixels(uint32_t displayId, void* buf, uint32_t bytes) {
    android::base::AutoLock lock(mLock);
    recordDisplay& r = mRecordDisplays[displayId];
    r.mIsCopying = true;
    lock.unlock();

    GLuint buffer = r.mBuffers[r.mMapCopyIndex];
    s_gles2.glBindBuffer(GL_COPY_READ_BUFFER, buffer);
    void* pixels = s_gles2.glMapBufferRange(GL_COPY_READ_BUFFER, 0, bytes,
                                            GL_MAP_READ_BIT);
    memcpy(buf, pixels, bytes);
    s_gles2.glUnmapBuffer(GL_COPY_READ_BUFFER);

    lock.lock();
    r.mIsCopying = false;
    lock.unlock();
}
