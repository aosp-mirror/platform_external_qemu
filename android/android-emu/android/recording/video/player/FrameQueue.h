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

#include <stddef.h>                                              // for size_t
#include <stdint.h>                                              // for int64_t

#include "android/recording/video/player/VideoPlayerWaitInfo.h"  // for Vide...

extern "C" {
#include <libavutil/frame.h>                                     // for AVFrame
#include <libavutil/rational.h>                                  // for AVRa...
}

namespace android {
namespace videoplayer {

class PacketQueue;
class VideoPlayer;

// Common struct for handling all types of decoded data and allocated render
// buffers
struct Frame {
    AVFrame* frame = nullptr;
    int serial = 0;

    // presentation timestamp for the frame
    double pts = 0.0;

    // estimated duration of the frame
    double duration = 0.0;

    AVRational sar = {0};

    // byte position of the frame in the input file
    int64_t pos = 0ll;
    bool allocated = false;
    bool reallocate = false;

    // buffer to hold the rgb pixels
    unsigned char* buf = nullptr;
    size_t len = 0;
    int headerlen = 0;

    // source height & width
    int width = 0;
    int height = 0;
};

#define VIDEO_PICTURE_QUEUE_SIZE 3
#define AUDIO_SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE \
    FFMAX(AUDIO_SAMPLE_QUEUE_SIZE, VIDEO_PICTURE_QUEUE_SIZE)

#define SDL_AUDIO_BUFFER_SIZE 1024
#define MAX_AUDIO_FRAME_SIZE 192000

#define MAX_AUDIOQ_SIZE (5 * 16 * 1024)
#define MAX_VIDEOQ_SIZE (5 * 256 * 1024)

// no AV sync correction is done if below the minimum AV sync threshold
#define AV_SYNC_THRESHOLD_MIN 0.04

// AV sync correction is done if above the maximum AV sync threshold
#define AV_SYNC_THRESHOLD_MAX 0.1

// If a frame duration is longer than this, it will not be duplicated to
// compensate AV sync
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1

// no AV correction is done if too big error
#define AV_NOSYNC_THRESHOLD 10.0

// a queue structure to keep decoded audio/video frames ready for rendering
class FrameQueue {
public:
    FrameQueue(VideoPlayer* player,
               PacketQueue* pktq,
               int max_size,
               bool keep_last);
    ~FrameQueue();

    void signalWait();
    Frame* peek();
    Frame* peekNext();
    Frame* peekLast();
    Frame* peekWritable();
    Frame* peekReadable();
    // no wait, return null if no data
    Frame* peekReadableNoWait();
    void push();
    void next();
    int prev();
    int numRemaining();
    int64_t lastPos();

    int size() const { return mSize; }

    VideoPlayerWaitInfo* getWaitInfo() { return &mWaitInfo; }

private:
    VideoPlayer* mPlayer = nullptr;
    PacketQueue* mPacketQueue = nullptr;

    Frame mQueue[FRAME_QUEUE_SIZE];
    int mRindex = 0;
    int mWindex = 0;
    int mSize = 0;
    int mMaxSize = 0;
    bool mKeepLast = 0;
    int mRindexShown = 0;

    VideoPlayerWaitInfo mWaitInfo;
};

}  // namespace videoplayer
}  // namespace android
