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
    virtual ~VideoPlayer();
    bool isRunning() const { return mRunning; }
    void stop();

    void decodeAudioThread();
    void decodeVideoThread();

    void scheduleRefresh(int delayMs);

private:
    int play(const char* filename);
    void adjustWindowSize(AVCodecContext* pVideoCodecCtx,
                          int* pWidth,
                          int* pHeight);
    void run();

    static bool isRealTimeFormat(AVFormatContext *s);
    void checkExternalClockSpeed();

    int queuePicture(AVFrame *src_frame, double pts, double duration, int64_t pos, int serial);
    void displayVideoFrame(Frame* vp);

    // get an audio frame from the decoded queue, and convert it to buffer
    int getConvertedAudioFrame();
    static void audioCallback(void* opaque, int avail);
    void internalStop();
    void cleanup();

    AvSyncMaster getMasterSyncType();
    double getMasterClock();

    int getVideoFrame(AVFrame *frame);

    double computeTargetDelay(double delay);
    double computeVideoFrameDuration(Frame *vp, Frame *nextvp);
    void updateVideoPts(double pts, int64_t pos, int serial);
    void videoRefresh(double *remaining_time);

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
    int mWidth;
    int mHeight;

    // pixel width and height of the video display window
    int mWindowWidth;
    int mWindowHeight;

    // video decoding stuff
    AVCodecContext* pVideoCodecCtx = nullptr;
    AVCodec* pVideoCodec = nullptr;

    // audio decoding stuff
    AVCodecContext* pAudioCodecCtx = nullptr;
    AVCodec* pAudioCodec = nullptr;

    SwsContext* img_convert_ctx = nullptr;
    SwrContext* swr_ctx = nullptr;

    Clock mAudioClock;
    Clock mVideoClock;
    Clock mExternalClock;

    std::unique_ptr<PacketQueue> mAudioQueue;
    std::unique_ptr<PacketQueue> mVideoQueue;

    // maximum duration of a frame
    // above this, we consider the jump a timestamp discontinuity
    double max_frame_duration = 10.0;

    std::unique_ptr<Decoder> mAudioDecoder;
    std::unique_ptr<Decoder> mVideoDecoder;

    std::unique_ptr<ThreadWrapper> mAudioDecodingThread;
    std::unique_ptr<ThreadWrapper> mVideoDecodingThread;

    AvSyncMaster av_sync_type = AvSyncMaster::Audio;

    double audio_clock = 0.0;
    int audio_clock_serial = 0;
    // used for AV difference average computation
    double audio_diff_cum = 0.0;
    double audio_diff_avg_coef = 0.0;
    double audio_diff_threshold = 0.0;
    int audio_diff_avg_count = 0;

    int frame_drops_early = 0;
    int frame_drops_late = 0;

    uint8_t *audio_buf = nullptr;
    uint8_t *audio_buf1 = nullptr;

    // size in bytes
    unsigned int audio_buf_size = 0;
    unsigned int audio_buf1_size = 0;

    // size in bytes
    int audio_buf_index = 0;
    int audio_write_buf_size = 0;

    double frame_timer = 0.0;
    double frame_last_returned_time = 0.0;
    double frame_last_filter_delay = 0.0;

    double last_vis_time = 0.0;

    // queue to store decoced audio frames
    std::unique_ptr<FrameQueue> mAudioFrameQueue = nullptr;

    // queue to store decoded video frames
    std::unique_ptr<FrameQueue> mVideoFrameQueue = nullptr;

    AudioOutputEngine* mAudioOutputEngine = nullptr;
    int64_t mAudioCallbackTime = 0ll;

    bool force_refresh = false;

    // drop frames when cpu is too slow
    int framedrop = -1;

    QTimer mTimer;

private slots:
    void videoRefreshTimer();

signals:
    void updateWidget();
    void videoFinished();
    void videoSetup(int lengthMsec);
};

} // namespace videoplayer
} // namespace android
