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

#include "android/recording/video/player/PacketQueue.h"

#include <libavcodec/avcodec.h>                              // for AVPacket
#include <libavutil/avutil.h>                                // for AV_NOPTS...
#include <libavutil/mem.h>                                   // for av_free
#include <stddef.h>                                          // for NULL

#include "android/base/Log.h"                                // for LOG, Log...
#include "android/base/synchronization/ConditionVariable.h"  // for Conditio...
#include "android/base/synchronization/Lock.h"               // for Lock
#include "android/recording/video/player/VideoPlayer.h"      // for VideoPlayer

namespace android {
namespace videoplayer {

//// PacketQueue implementations

AVPacket PacketQueue::sFlushPkt = {0};

void PacketQueue::init() {
    av_init_packet(&sFlushPkt);
    sFlushPkt.data = (uint8_t*)&sFlushPkt;
}

PacketQueue::PacketQueue(VideoPlayer* player) : mPlayer(player) {}

PacketQueue::~PacketQueue() {
    flush();
}

void PacketQueue::start() {
    VideoPlayerWaitInfo* pwi = &mWaitInfo;
    pwi->lock.lock();

    mAbortRequest = false;
    internalPut(&sFlushPkt);

    pwi->done = true;
    pwi->cvDone.signalAndUnlock(&pwi->lock);
}

void PacketQueue::abort() {
    VideoPlayerWaitInfo* pwi = &mWaitInfo;
    pwi->lock.lock();
    mAbortRequest = true;
    pwi->done = true;
    pwi->cvDone.signalAndUnlock(&pwi->lock);
}

int PacketQueue::internalPut(AVPacket* pkt) {
    if (mAbortRequest) {
        return -1;
    }

    CustomAVPacketList* pkt1 =
            (CustomAVPacketList*)av_malloc(sizeof(CustomAVPacketList));
    if (pkt1 == nullptr) {
        return -1;
    }

    pkt1->pkt = *pkt;
    pkt1->next = nullptr;
    if (pkt == &sFlushPkt) {
        mSerial++;
    }
    pkt1->serial = mSerial;

    if (mLastPkt == nullptr) {
        mFirstPkt = pkt1;
    } else {
        mLastPkt->next = pkt1;
    }
    mLastPkt = pkt1;
    mNumPkts++;
    mSize += pkt1->pkt.size + sizeof(*pkt1);

    return 0;
}

// insert a new packet
int PacketQueue::put(AVPacket* pkt) {
    VideoPlayerWaitInfo* pwi = &mWaitInfo;
    pwi->lock.lock();

    int ret = internalPut(pkt);

    pwi->done = true;
    pwi->cvDone.signalAndUnlock(&pwi->lock);

    if (pkt != &sFlushPkt && ret < 0) {
        av_packet_unref(pkt);
    }

    return ret;
}

// insert a null packet
// marks the end of the packet queue, stop processing when met
int PacketQueue::putNullPacket(int stream_index) {
    AVPacket pkt1, *pkt = &pkt1;
    av_init_packet(pkt);
    pkt->data = nullptr;
    pkt->size = 0;
    pkt->stream_index = stream_index;
    return put(pkt);
}

void PacketQueue::flush() {
    VideoPlayerWaitInfo* pwi = &mWaitInfo;
    pwi->lock.lock();

    CustomAVPacketList* pkt1;
    for (CustomAVPacketList* pkt = mFirstPkt; pkt; pkt = pkt1) {
        pkt1 = pkt->next;
        av_packet_unref(&pkt->pkt);
        av_freep(&pkt);
    }
    mLastPkt = nullptr;
    mFirstPkt = nullptr;
    mNumPkts = 0;
    mSize = 0;

    internalPut(&sFlushPkt);

    pwi->lock.unlock();
}

// retrieve the front packet, usually for decoding
// return < 0 if aborted, 0 if no packet and > 0 if packet
int PacketQueue::get(AVPacket* pkt, bool blocking, int* serial) {
    int ret = 0;
    VideoPlayerWaitInfo* pwi = &mWaitInfo;

    pwi->lock.lock();

    while (true) {
        if (!mPlayer->isRunning() || mAbortRequest) {
            ret = -1;
            break;
        }

        CustomAVPacketList* pkt1 = mFirstPkt;
        if (pkt1 != nullptr) {
            mFirstPkt = pkt1->next;
            if (mFirstPkt == nullptr) {
                mLastPkt = NULL;
            }
            mNumPkts--;
            mSize -= pkt1->pkt.size + sizeof(*pkt1);
            if (pkt1->pkt.size == 0 && pkt1->pkt.data == nullptr) {
                ret = -2;  // EOF packet
                break;
            }
            *pkt = pkt1->pkt;
            if (serial) {
                *serial = pkt1->serial;
            }
            av_free(pkt1);
            ret = 1;
            break;
        } else if (!blocking) {
            ret = 0;
            break;
        } else {
            pwi->done = false;
            while (!pwi->done) {
                pwi->cvDone.wait(&pwi->lock);
            }
        }
    }

    pwi->lock.unlock();

    return ret;
}

// signal and unlock blocking wait
void PacketQueue::signalWait() {
    VideoPlayerWaitInfo* pwi = &mWaitInfo;
    pwi->lock.lock();
    pwi->done = true;
    pwi->cvDone.signalAndUnlock(&pwi->lock);
}

int64_t PacketQueue::getEnqueuedPktDuration() {
    if (mFirstPkt == nullptr || mLastPkt == nullptr) {
        return 0L;
    } else if (mFirstPkt->pkt.pts == AV_NOPTS_VALUE ||
               mLastPkt->pkt.pts == AV_NOPTS_VALUE) {
        LOG(WARNING) << "Packet has no valid PTS value!";
        return -1;
    } else {
        return mLastPkt->pkt.pts - mFirstPkt->pkt.pts;
    }
}

}  // namespace videoplayer
}  // namespace android
