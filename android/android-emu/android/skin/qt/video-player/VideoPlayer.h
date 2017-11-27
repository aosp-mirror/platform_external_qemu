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

// Portions of code is from a tutorial by dranger at gmail dot com:
//   http://dranger.com/ffmpeg
// licensed under the Creative Commons Attribution-Share Alike 2.5 License.

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

#include "android/emulation/AudioOutputEngine.h"
#include "android/skin/qt/video-player/Clock.h"
#include "android/skin/qt/video-player/Decoder.h"
#include "android/skin/qt/video-player/FrameQueue.h"
#include "android/skin/qt/video-player/PacketQueue.h"
#include "android/skin/qt/video-player/VideoPlayerWaitInfo.h"
#include "android/skin/qt/video-player/VideoPlayerWidget.h"

#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/threads/Thread.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#include "libavutil/samplefmt.h"
#include "libavutil/time.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}

#include <QtCore/qthread.h>
#include <QTimer>

#include <memory>

using android::emulation::AudioOutputEngine;

namespace android {
namespace videoplayer {

enum class AvSyncMaster {
    // sync to audio if present, default choice
    Audio,

    // sync to video
    Video,

    // synchronize to an external clock
    External
};

class VideoPlayer : public QThread {
    Q_OBJECT

public:
    VideoPlayer(VideoPlayerWidget* widget, std::string videoFile);
    virtual ~VideoPlayer() = default;
    bool isRunning() const { return mRunning; }
    void stop();

    void decodeAudioThread();
    void decodeVideoThread();

    void scheduleRefresh(int delayMs);

private:
    int play(const char* filename);
    void adjustWindowSize(AVCodecContext* c, int* pWidth, int* pHeight);
    void run();

    static bool isRealTimeFormat(AVFormatContext* s);
    void checkExternalClockSpeed();

    int queuePicture(AVFrame* src_frame,
                     double pts,
                     double duration,
                     int64_t pos,
                     int serial);
    void displayVideoFrame(Frame* vp);

    // get an audio frame from the decoded queue, and convert it to buffer
    int getConvertedAudioFrame();
    static void audioCallback(void* opaque, int avail);
    void internalStop();
    void cleanup();

    AvSyncMaster getMasterSyncType();
    double getMasterClock();

    int getVideoFrame(AVFrame* frame);

    double computeTargetDelay(double delay);
    double computeVideoFrameDuration(Frame* vp, Frame* nextvp);
    void updateVideoPts(double pts, int64_t pos, int serial);
    void videoRefresh(double* remaining_time);

private:
    std::string mVideoFile;

    VideoPlayerWidget* mWidget = nullptr;

    // this is a real time stream, e.g., rtsp
    bool mRealTime = false;

    bool mRunning = false;
    bool mPaused = false;

    AVFormatContext* mFormatCtx = nullptr;
    int mAudioStreamIdx = -1;
    int mVideoStreamIdx = -1;

    // video data width and height, may use this to resize window
    int mWidth = 0;
    int mHeight = 0;

    // pixel width and height of the video display window
    int mWindowWidth = 0;
    int mWindowHeight = 0;

    // video decoding stuff
    AVCodecContext* mVideoCodecCtx = nullptr;
    AVCodec* mVideoCodec = nullptr;

    // audio decoding stuff
    AVCodecContext* mAudioCodecCtx = nullptr;
    AVCodec* mAudioCodec = nullptr;

    SwsContext* mImgConvertCtx = nullptr;
    SwrContext* mSwrCtx = nullptr;

    Clock mAudioClock;
    Clock mVideoClock;
    Clock mExternalClock;

    // we use this condition to control not reading too much
    // data into the packet queues
    VideoPlayerWaitInfo mContinueReadWaitInfo;

    std::unique_ptr<PacketQueue> mAudioQueue;
    std::unique_ptr<PacketQueue> mVideoQueue;

    std::unique_ptr<Decoder> mAudioDecoder;
    std::unique_ptr<Decoder> mVideoDecoder;

    std::unique_ptr<DecoderThread> mAudioDecodingThread;
    std::unique_ptr<DecoderThread> mVideoDecodingThread;

    // queue to store decoced audio frames
    std::unique_ptr<FrameQueue> mAudioFrameQueue = nullptr;

    // queue to store decoded video frames
    std::unique_ptr<FrameQueue> mVideoFrameQueue = nullptr;

    AvSyncMaster mAvSyncType = AvSyncMaster::Audio;

    // maximum duration of a frame
    // above this, we consider the jump a timestamp discontinuity
    double mMaxFrameDuration = 10.0;

    double mFrameTimer = 0.0;
    double mLastVisibleTime = 0.0;

    // stats
    int mFrameDropsEarly = 0;
    int mFrameDropsLate = 0;

    double mAudioClockValue = 0.0;
    int mAudioClockSerial = 0;

    // double buffering to move audio around
    uint8_t* mAudioBuf = nullptr;
    uint8_t* mAudioBuf1 = nullptr;

    // size in bytes
    unsigned int mAudioBufSize = 0;
    unsigned int mAudioBuf1Size = 0;

    // index and size are in bytes unit
    int mAudioBufIndex = 0;
    int mAudioWriteBufSize = 0;

    AudioOutputEngine* mAudioOutputEngine = nullptr;
    int64_t mAudioCallbackTime = 0ll;

    bool mForceRefresh = false;

    // drop frames when cpu is too slow
    int mFrameDrop = -1;

    QTimer mTimer;

private slots:
    void videoRefreshTimer();

signals:
    void updateWidget();
    void videoFinished();
    void videoSetup(int lengthMsec);
};

}  // namespace videoplayer
}  // namespace android
