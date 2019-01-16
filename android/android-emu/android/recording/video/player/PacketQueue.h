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

#include "android/recording/video/player/VideoPlayerWaitInfo.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#include "libavutil/time.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}

namespace android {
namespace videoplayer {

class VideoPlayer;

struct CustomAVPacketList {
    AVPacket pkt;
    CustomAVPacketList* next;
    int serial;
};

// a queue structure to keep audio/video packets read from a video file
class PacketQueue {
public:
    PacketQueue(VideoPlayer* player);
    ~PacketQueue();

    static void init();

    void start();

    void abort();

    // insert a new packet
    int put(AVPacket* pkt);

    int putNullPacket(int stream_index);

    void flush();

    // retrieve the front packet, usually for decoding
    int get(AVPacket* pkt, bool blocking, int* serial);

    // signal and unlock blocking wait
    void signalWait();

    bool isAbort() const { return mAbortRequest; }

    int getNumPackets() const { return mNumPkts; }

    int getSize() const { return mSize; }

    int getSerial() const { return mSerial; }

    int* getSerialPtr() { return &mSerial; }

private:
    int internalPut(AVPacket* pkt);

public:
    // this is special packet to mark a flush is needed
    // mainly for future use of seeking a video file
    static AVPacket sFlushPkt;

private:
    CustomAVPacketList* mFirstPkt = nullptr;
    CustomAVPacketList* mLastPkt = nullptr;
    int mNumPkts = 0;
    int mSize = 0;
    // must call start() method to enable the queue
    bool mAbortRequest = true;
    int mSerial = 0;

    VideoPlayerWaitInfo mWaitInfo;
    VideoPlayer* mPlayer = nullptr;
};

}  // namespace videoplayer
}  // namespace android
