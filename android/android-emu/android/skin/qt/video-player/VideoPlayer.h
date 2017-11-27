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

class VideoPlayer;

struct Clock {
    // clock base
    double pts;

    // clock base minus time at which we updated the clock
    double pts_drift;
    double last_updated;
    double speed;

    // clock is based on a packet with this serial
    int serial;
    int paused;

    // pointer to the current packet queue serial, used for obsolete clock
    // detection
    int* queue_serial;
};

struct VideoPicture {
    // buffer to hold the rgb pixels
    unsigned char* buf = nullptr;
    size_t len = 0;

    int headerlen = 0;

    // source height & width
    int width = 0;
    int height = 0;
    bool allocated = false;
    double pts = 0.0;
};

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
    VideoPlayerWidget* mWidget;
    int play(const char* filename);
    void adjustWindowSize(AVCodecContext* pVideoCodecCtx,
                          int* pWidth,
                          int* pHeight);
    void run();
    int queuePicture(AVFrame* pFrame, double pts);
    double synchronizeVideo(AVFrame* frame, double pts);
    void displayVideoFrame(VideoPicture* vp);

    // get an audio frame from the decoded queue, and convert it to buffer
    int getConvertedAudioFrame();
    static void audioCallback(void* opaque, int avail);
    void internalStop();
    void cleanup();

private:
    friend class FrameQueue;

    std::string mVideoFile;
    bool mRunning = false;

    AVFormatContext* pFormatCtx = nullptr;
    int mAudioStreamIdx = -1;
    int mVideoStreamIdx = -1;

    // video decoding stuff
    AVCodecContext* pVideoCodecCtx = nullptr;
    AVCodec* pVideoCodec = nullptr;
    AVFrame* pVideoFrame = nullptr;
    AVFrame* pFrameRGB = nullptr;

    // audio decoding stuff
    AVCodecContext* pAudioCodecCtx = nullptr;
    AVCodec* pAudioCodec = nullptr;
    AVFrame* pAudioFrame = nullptr;

    int64_t start_pts = 0;
    AVRational start_pts_tb = {0};
    int64_t next_pts = 0;
    AVRational next_pts_tb = {0};

    int numBytes;
    uint8_t* buffer = nullptr;
    SwsContext* img_convert_ctx = nullptr;
    SwrContext* swr_ctx = nullptr;
    uint8_t* resampleBuffer = nullptr;

    int dst_nb_samples = 4096;
    int current_audio_wait = 0;
    int audio_frame_count = 0;

    std::unique_ptr<PacketQueue> mAudioQueue;
    std::unique_ptr<PacketQueue> mVideoQueue;

    std::unique_ptr<Decoder> mAudioDecoder;
    std::unique_ptr<Decoder> mVideoDecoder;

    std::unique_ptr<ThreadWrapper> mAudioDecodingThread;
    std::unique_ptr<ThreadWrapper> mVideoDecodingThread;

    VideoPicture pictq[VIDEO_PICTURE_QUEUE_SIZE /*FRAME_QUEUE_SIZE*/];
    int pictq_size = 0;
    int pictq_rindex = 0;
    int pictq_windex = 0;
    VideoPlayerWaitInfo mFrameQueueWaitInfo;

    double audio_clock = 0.0;
    int audio_clock_serial = 0;
    double audio_diff_avg_coef = 0.0;
    double audio_diff_threshold = 0.0;
    int audio_diff_avg_count = 0;

    uint8_t *audio_buf = nullptr;
    uint8_t *audio_buf1 = nullptr;

    // size in bytes
    unsigned int audio_buf_size = 0;
    unsigned int audio_buf1_size = 0;

    // size in bytes
    int audio_buf_index = 0;
    int audio_write_buf_size = 0;

    double frame_timer = 0.0;
    double frame_last_pts = 0.0;
    double frame_last_delay = 0.0;
    double video_clock = 0.0;  ///< pts of last decoded frame / predicted pts of
                               ///< next decoded frame

    // queue to store decoced audio frames
    std::unique_ptr<FrameQueue> mAudioFrameQueue = nullptr;

    // queue to store decoded video frames
    std::unique_ptr<FrameQueue> mVideoFrameQueue = nullptr;

    AudioOutputEngine* mAudioOutputEngine = nullptr;
    int64_t mAudioCallbackTime = 0ll;

    // stats
    int mNumVideoFrames = 0;
    int mNumVideoPicts = 0;

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
