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

#pragma once

#include "android/emulation/AudioOutputEngine.h"
#include "android/skin/qt/video-player/VideoPlayerWidget.h"

#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/threads/Thread.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#include "libavutil/time.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}

#include <QtCore/qthread.h>
#include <QTimer>

#include <memory>

using android::emulation::AudioOutputEngine;

struct AVFormatContext;
struct AVCodec;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct AVPacketList;
struct AVRational;
struct SwrContext;
struct SwsContext;

namespace android {
namespace videoplayer {

class VideoPlayer;

struct VideoPlayerWaitInfo {
    android::base::Lock lock;  // protects other parts of this struct
    bool done = false;
    android::base::ConditionVariable cvDone;
};

class ThreadWrapper : public android::base::Thread {
public:
    ThreadWrapper(VideoPlayer* player, int kind)
        : mPlayer(player), mKind(kind) {}

    // Main thread function
    virtual intptr_t main() override;

private:
    VideoPlayer* mPlayer;
    int mKind;
};

// a queue structure to keep audio/video packets read from a video file
class PacketQueue {
public:
    PacketQueue(VideoPlayer* player);
    ~PacketQueue();

    // insert a new packet
    int put(AVPacket* pkt);

    // retrieve the front packet, usually for decoding
    int get(AVPacket* pkt, bool blocking);

    // signal and unlock blocking wait
    void signalWait();

private:
    friend class FrameQueue;

    AVPacketList* mFirstPkt = nullptr;
    AVPacketList* mLastPkt = nullptr;
    int mNumPkts = 0;
    int mSize = 0;
    int mAbortRequest = 0;
    int mSerial = 0;

    VideoPlayerWaitInfo mWaitInfo;
    VideoPlayer* mPlayer;
};

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

// Common struct for handling all types of decoded data and allocated render
// buffers
struct Frame {
    AVFrame* frame;
    int serial;

    // presentation timestamp for the frame
    double pts;

    // estimated duration of the frame
    double duration;

    // byte position of the frame in the input file
    int64_t pos;
    int allocated;
    int reallocate;
    int width;
    int height;
    AVRational sar;
};

struct VideoPicture {
    // buffer to hold the rgb pixels
    unsigned char* buf = nullptr;
    int len = 0;

    int headerlen = 0;

    // source height & width
    int width = 0;
    int height = 0;
    bool allocated = false;
    double pts = 0.0;
};

#define VIDEO_PICTURE_QUEUE_SIZE 2
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, VIDEO_PICTURE_QUEUE_SIZE)

#define SDL_AUDIO_BUFFER_SIZE 1024
#define MAX_AUDIO_FRAME_SIZE 192000

#define MAX_AUDIOQ_SIZE (5 * 16 * 1024)
#define MAX_VIDEOQ_SIZE (5 * 256 * 1024)

#define AV_SYNC_THRESHOLD 0.01
#define AV_NOSYNC_THRESHOLD 10.0

// a queue structure to keep decoded audio/video frames ready for rendering
class FrameQueue {
public:
    FrameQueue(VideoPlayer* player,
               PacketQueue* pktq,
               int max_size,
               bool keep_last);
    ~FrameQueue();

    void signal();
    Frame* peek();
    Frame* peekNext();
    Frame* peekLast();
    Frame* peekWritable();
    Frame* peekReadable();
    void push();
    void next();
    int prev();
    int numRemaining();
    int64_t lastPos();

private:
    VideoPlayer* mPlayer = nullptr;
    PacketQueue* mPktq = nullptr;

    Frame mQueue[FRAME_QUEUE_SIZE];
    int mRindex = 0;
    int mWindex = 0;
    int mSize = 0;
    int mMaxSize = 0;
    bool mKeepLast = 0;
    int mRindexShown = 0;

    VideoPlayerWaitInfo mWaitInfo;
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
    AVCodecContext* pVideoCodecCtx = nullptr;
    AVCodec* pVideoCodec = nullptr;
    AVFrame* pVideoFrame = nullptr;
    AVFrame* pFrameRGB = nullptr;

    int numBytes;
    uint8_t* buffer = nullptr;
    SwsContext* img_convert_ctx = nullptr;

    AVCodecContext* pAudioCodecCtx = nullptr;
    AVCodec* pAudioCodec = nullptr;
    AVFrame* pAudioFrame = nullptr;

    SwrContext* swr_ctx = nullptr;
    uint8_t* resampleBuffer = nullptr;

    int dst_nb_samples = 4096;
    int current_audio_wait = 0;
    int audio_frame_count = 0;

    std::unique_ptr<PacketQueue> mAudioQueue;
    std::unique_ptr<PacketQueue> mVideoQueue;

    std::unique_ptr<ThreadWrapper> mAudioDecodingThread;
    std::unique_ptr<ThreadWrapper> mVideoDecodingThread;

    VideoPicture pictq[VIDEO_PICTURE_QUEUE_SIZE /*FRAME_QUEUE_SIZE*/];
    int pictq_size = 0;
    int pictq_rindex = 0;
    int pictq_windex = 0;
    VideoPlayerWaitInfo mFrameQueueWaitInfo;

    double audio_clock = 0.0;
    double frame_timer = 0.0;
    double frame_last_pts = 0.0;
    double frame_last_delay = 0.0;
    double video_clock = 0.0;  ///< pts of last decoded frame / predicted pts of
                               ///< next decoded frame

    // q queue to store decoded video frames
    FrameQueue* mPictureQueue = nullptr;

    QTimer mTimer;

    AudioOutputEngine* mAudioOutputEngine = nullptr;
    VideoPlayerWaitInfo mAudioAvailableWaitInfo;
    VideoPlayerWaitInfo mAudioBufferDoneWaitInfo;

    // stats
    int mNumVideoFrames = 0;
    int mNumVideoPicts = 0;

private slots:
    void videoRefreshTimer();

signals:
    void updateWidget();
    void videoFinished();
    void videoSetup(int lengthMsec);
};

} // namespace videoplayer
} // namespace android
