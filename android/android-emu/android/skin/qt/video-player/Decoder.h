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

#pragma once

#include "android/skin/qt/video-player/VideoPlayerWaitInfo.h"
#include "android/base/threads/Thread.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#include "libavutil/time.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}

#include <memory>

namespace android {
namespace videoplayer {

class VideoPlayer;
class PacketQueue;

enum class ThreadType {
    AudioDecoding,
    VideoDecoding
};

class DecoderThread : public android::base::Thread {
public:
    DecoderThread(VideoPlayer* player, ThreadType type)
    : mPlayer(player), mType(type) {}

    // Main thread function
    virtual intptr_t main() override;

private:
    VideoPlayer* mPlayer;
    ThreadType mType;
};

// a decoder class to decode audio/video packets
class Decoder {
public:
    Decoder(VideoPlayer* player,
            AVCodecContext* avctx,
            PacketQueue* queue,
            VideoPlayerWaitInfo* empty_queue_cond);
    ~Decoder();

    int getPktSerial() const {
        return mPktSerial;
    }

    int decodeFrame(AVFrame *frame);

    void startThread(ThreadType type);

    void abort();

    void waitToFinish();

private:
    // the current packet being decoded
    AVPacket mPkt = {0};
    // a packet to store left overs between decoding runs
    AVPacket mTmpPkt = {0};
    int mPktSerial = 0;
    bool mFinished = false;
    bool mPacketPending = false;
    int64_t mStartPts = 0ll;
    AVRational mStartPtsTb = {0};
    int64_t mNextPts = {0};
    AVRational mNextPtsTb = {0};
    VideoPlayer* mPlayer = nullptr;
    AVCodecContext* mAvCtx = nullptr;
    PacketQueue* mPacketQueue = nullptr;
    // this condition is to control packet reading speed
    // we don't want to use too much memory to keep packets
    // the main reading loop will wait when queue is full
    VideoPlayerWaitInfo* mEmptyQueueCond = nullptr;
    std::unique_ptr<DecoderThread> mThread = nullptr;
};

} // namespace videoplayer
} // namespace android
