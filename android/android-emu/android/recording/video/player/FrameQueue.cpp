// Copyright (C) 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Copyright (c) 2003 Fabrice Bellard
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "android/recording/video/player/FrameQueue.h"

#include "android/base/synchronization/ConditionVariable.h"  // for Conditio...
#include "android/base/synchronization/Lock.h"               // for Lock
#include "android/recording/video/player/PacketQueue.h"      // for PacketQueue
#include "android/recording/video/player/VideoPlayer.h"      // for VideoPlayer

namespace android {
namespace videoplayer {

// FrameQueue implementations

FrameQueue::FrameQueue(VideoPlayer* player,
                       PacketQueue* pktq,
                       int max_size,
                       bool keep_last)
    : mPlayer(player), mPacketQueue(pktq), mKeepLast(keep_last) {
    mMaxSize = FFMIN(max_size, FRAME_QUEUE_SIZE);
    for (int i = 0; i < mMaxSize; i++) {
        mQueue[i].frame = av_frame_alloc();
    }
}

FrameQueue::~FrameQueue() {
    for (int i = 0; i < mMaxSize; i++) {
        Frame* vp = &mQueue[i];
        av_frame_unref(vp->frame);
        av_frame_free(&vp->frame);
        if (vp->buf != nullptr) {
            delete[] vp->buf;
            vp->buf = nullptr;
        }
    }
}

void FrameQueue::signalWait() {
    VideoPlayerWaitInfo* pwi = &mWaitInfo;
    pwi->lock.lock();
    pwi->done = true;
    pwi->cvDone.signalAndUnlock(&pwi->lock);
}

Frame* FrameQueue::peek() {
    return &mQueue[(mRindex + mRindexShown) % mMaxSize];
}

Frame* FrameQueue::peekNext() {
    return &mQueue[(mRindex + mRindexShown + 1) % mMaxSize];
}

Frame* FrameQueue::peekLast() {
    return &mQueue[mRindex];
}

Frame* FrameQueue::peekWritable() {
    // wait until we have space to put a new frame
    VideoPlayerWaitInfo* pwi = &mWaitInfo;
    pwi->lock.lock();
    pwi->done = false;
    while (mSize >= mMaxSize && !pwi->done && mPlayer->isRunning() &&
           !mPacketQueue->isAbort()) {
        pwi->cvDone.wait(&pwi->lock);
    }
    pwi->lock.unlock();

    if (!mPlayer->isRunning() || mPacketQueue->isAbort())
        return nullptr;

    return &mQueue[mWindex];
}

Frame* FrameQueue::peekReadable() {
    // wait until we have a readable new frame
    VideoPlayerWaitInfo* pwi = &mWaitInfo;
    pwi->lock.lock();
    pwi->done = false;
    while ((mSize - mRindexShown) <= 0 && !pwi->done && mPlayer->isRunning() &&
           !mPacketQueue->isAbort()) {
        pwi->cvDone.wait(&pwi->lock);
    }
    pwi->lock.unlock();

    if (!mPlayer->isRunning() || mPacketQueue->isAbort()) {
        return nullptr;
    }

    return &mQueue[(mRindex + mRindexShown) % mMaxSize];
}

// no wait, return null if no data
Frame* FrameQueue::peekReadableNoWait() {
    bool empty = false;
    VideoPlayerWaitInfo* pwi = &mWaitInfo;
    pwi->lock.lock();
    empty = (mSize - mRindexShown) <= 0;
    if (empty || !mPlayer->isRunning() || mPacketQueue->isAbort()) {
	pwi->lock.unlock();
        return nullptr;
    }

    pwi->lock.unlock();

    return &mQueue[(mRindex + mRindexShown) % mMaxSize];
}

void FrameQueue::push() {
    if (++mWindex == mMaxSize) {
        mWindex = 0;
    }

    VideoPlayerWaitInfo* pwi = &mWaitInfo;
    pwi->lock.lock();
    mSize++;
    pwi->cvDone.signalAndUnlock(&pwi->lock);
}

void FrameQueue::next() {
    if (mKeepLast && !mRindexShown) {
        mRindexShown = 1;
        return;
    }
    av_frame_unref(mQueue[mRindex].frame);
    if (++mRindex == mMaxSize) {
        mRindex = 0;
    }

    VideoPlayerWaitInfo* pwi = &mWaitInfo;
    pwi->lock.lock();
    mSize--;
    pwi->cvDone.signalAndUnlock(&pwi->lock);
}

// jump back to the previous frame if available by resetting rindex_shown
int FrameQueue::prev() {
    int ret = mRindexShown;
    mRindexShown = 0;
    return ret;
}

// return the number of unrendered frames in the queue
int FrameQueue::numRemaining() {
    return mSize - mRindexShown;
}

// return last shown position
int64_t FrameQueue::lastPos() {
    Frame* fp = &mQueue[mRindex];
    if (mRindexShown && fp->serial == mPacketQueue->getSerial()) {
        return fp->pos;
    } else {
        return -1;
    }
}

}  // namespace videoplayer
}  // namespace android
