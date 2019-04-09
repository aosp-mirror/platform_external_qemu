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

#include "android/recording/video/player/VideoPlayer.h"

#include "android/base/Log.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/threads/FunctorThread.h"
#include "android/base/threads/Thread.h"
#include "android/emulation/AudioOutputEngine.h"
#include "android/recording/video/player/Clock.h"
#include "android/recording/video/player/FrameQueue.h"
#include "android/recording/video/player/PacketQueue.h"
#include "android/recording/AVScopedPtr.h"
#include "android/utils/debug.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#include "libavutil/samplefmt.h"
#include "libavutil/time.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}

#include <cmath>

#define D(...) VERBOSE_PRINT(record, __VA_ARGS__)

using android::emulation::AudioOutputEngine;
using android::recording::AVScopedPtr;
using android::recording::makeAVScopedPtr;

namespace android {
namespace videoplayer {

// Forward declarations
class VideoPlayerImpl;

// a decoder class to decode audio/video packets
class Decoder {
public:
    Decoder(VideoPlayerImpl* player,
            AVCodecContext* avctx,
            PacketQueue* queue,
            FrameQueue* frameQueue,
            VideoPlayerWaitInfo* empty_queue_cond);
    virtual ~Decoder();

    int getPktSerial() const { return mPktSerial; }

    virtual void workerThreadFunc() = 0;

    // start the work thread to perfrom decoding
    void start();

    // abort the decoding
    void abort();

    // wait for the worker thread to finish
    void wait();

protected:
    int fetchPacket();

protected:
    // the current packet being decoded
    AVPacket mPkt = {0};
    // a packet to store left overs between decoding runs
    AVPacket mLeftoverPkt = {0};
    int mPktSerial = 0;
    bool mFinished = false;
    bool mPacketPending = false;
    int64_t mStartPts = 0ll;
    AVRational mStartPtsTb = {0};
    int64_t mNextPts = {0};
    AVRational mNextPtsTb = {0};
    VideoPlayerImpl* mPlayer = nullptr;
    AVCodecContext* mAvCtx = nullptr;
    PacketQueue* mPacketQueue = nullptr;
    FrameQueue* mFrameQueue = nullptr;
    // this condition is to control packet reading speed
    // we don't want to use too much memory to keep packets
    // the main reading loop will wait when queue is full
    VideoPlayerWaitInfo* mEmptyQueueCond = nullptr;

    // A separate worker thread for the decoder
    base::FunctorThread mWorkerThread;
};

class AudioDecoder : public Decoder {
public:
    AudioDecoder(VideoPlayerImpl* player,
                 AVCodecContext* avctx,
                 PacketQueue* queue,
                 FrameQueue* frameQueue,
                 VideoPlayerWaitInfo* empty_queue_cond)
        : Decoder(player, avctx, queue, frameQueue, empty_queue_cond) {}

    virtual void workerThreadFunc();

private:
    // real function to use ffmpeg api to decode an audio frame
    int decodeAudioFrame(AVFrame* frame);
};

class VideoDecoder : public Decoder {
public:
    VideoDecoder(VideoPlayerImpl* player,
                 AVCodecContext* avctx,
                 PacketQueue* queue,
                 FrameQueue* frameQueue,
                 VideoPlayerWaitInfo* empty_queue_cond)
        : Decoder(player, avctx, queue, frameQueue, empty_queue_cond) {}

    virtual void workerThreadFunc();

private:
    // real function to use ffmpeg api to decode an audio frame
    int decodeVideoFrame(AVFrame* frame);
    int getVideoFrame(AVFrame* frame);
};

enum class AvSyncMaster {
    // sync to audio if present, default choice
    Audio,

    // sync to video
    Video,

    // synchronize to an external clock
    External
};

class VideoPlayerImpl : public VideoPlayer {
public:
    VideoPlayerImpl(std::string videoFile,
                    VideoPlayerRenderTarget* renderTarget,
                    std::unique_ptr<VideoPlayerNotifier> notifier);
    virtual ~VideoPlayerImpl();

    virtual void start();
    virtual void stop();
    virtual bool isRunning() const { return mRunning; }
    virtual void scheduleRefresh(int delayMs);
    virtual void videoRefresh();

    AvSyncMaster getMasterSyncType();
    double getMasterClock();

    int queuePicture(AVFrame* src_frame,
                     double pts,
                     double duration,
                     int64_t pos,
                     int serial);

private:
    int play();
    void adjustWindowSize(AVCodecContext* c, int* pWidth, int* pHeight);

    static bool isRealTimeFormat(AVFormatContext* s);
    void checkExternalClockSpeed();
    void displayVideoFrame(Frame* vp);

    // get an audio frame from the decoded queue, and convert it to buffer
    int getConvertedAudioFrame();
    static void audioCallback(void* opaque, int avail);
    void internalStop();
    void cleanup();

    double computeTargetDelay(double delay);
    double computeVideoFrameDuration(Frame* vp, Frame* nextvp);
    void updateVideoPts(double pts, int64_t pos, int serial);
    void refreshFrame(double* remaining_time);

    void workerThreadFunc();

public:
    AVScopedPtr<AVFormatContext> mFormatCtx;
    int mAudioStreamIdx = -1;
    int mVideoStreamIdx = -1;

    // video data width and height, may use this to resize window
    int mWidth = 0;
    int mHeight = 0;

    Clock mAudioClock;
    Clock mVideoClock;
    Clock mExternalClock;

    // stats
    int mFrameDropsEarly = 0;
    int mFrameDropsLate = 0;

    // drop frames when cpu is too slow
    int mFrameDrop = -1;

private:
    std::string mVideoFile;
    VideoPlayerRenderTarget* mRenderTarget = nullptr;
    std::unique_ptr<VideoPlayerNotifier> mNotifier;

    // this is a real time stream, e.g., rtsp
    bool mRealTime = false;

    bool mRunning = false;
    bool mPaused = false;

    // pixel width and height of the video display window
    int mWindowWidth = 0;
    int mWindowHeight = 0;

    // video decoding stuff
    AVScopedPtr<AVCodecContext> mVideoCodecCtx;
    AVCodec* mVideoCodec = nullptr;

    // audio decoding stuff
    AVScopedPtr<AVCodecContext> mAudioCodecCtx;
    AVCodec* mAudioCodec = nullptr;

    SwsContext* mImgConvertCtx = nullptr;
    SwrContext* mSwrCtx = nullptr;

    // we use this condition to control not reading too much
    // data into the packet queues
    VideoPlayerWaitInfo mContinueReadWaitInfo;

    std::unique_ptr<PacketQueue> mAudioQueue;
    std::unique_ptr<PacketQueue> mVideoQueue;

    std::unique_ptr<AudioDecoder> mAudioDecoder;
    std::unique_ptr<VideoDecoder> mVideoDecoder;

    // queue to store decoded audio frames
    std::unique_ptr<FrameQueue> mAudioFrameQueue = nullptr;

    // queue to store decoded video frames
    std::unique_ptr<FrameQueue> mVideoFrameQueue = nullptr;

    AvSyncMaster mAvSyncType = AvSyncMaster::Audio;

    // maximum duration of a frame
    // above this, we consider the jump a timestamp discontinuity
    double mMaxFrameDuration = 10.0;

    double mFrameTimer = 0.0;
    double mLastVisibleTime = 0.0;

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

    // A separate worker thread for the player
    base::FunctorThread mWorkerThread;
};

// Decoder implementations

Decoder::Decoder(VideoPlayerImpl* player,
                 AVCodecContext* avctx,
                 PacketQueue* queue,
                 FrameQueue* frameQueue,
                 VideoPlayerWaitInfo* empty_queue_cond)
    : mPlayer(player),
      mAvCtx(avctx),
      mPacketQueue(queue),
      mFrameQueue(frameQueue),
      mEmptyQueueCond(empty_queue_cond),
      mWorkerThread([this]() { workerThreadFunc(); }) {}

Decoder::~Decoder() {
    av_packet_unref(&mPkt);
}

int Decoder::fetchPacket() {
    if (!mPacketPending || mPacketQueue->getSerial() != mPktSerial) {
        AVPacket pkt;
        do {
            if (mPacketQueue->getNumPackets() == 0 &&
                mEmptyQueueCond != nullptr) {
                mEmptyQueueCond->lock.lock();
                mEmptyQueueCond->done = true;
                mEmptyQueueCond->cvDone.signalAndUnlock(&mEmptyQueueCond->lock);
            }
            if (mPacketQueue->get(&pkt, true, &mPktSerial) < 0) {
                return -1;
            }
            if (pkt.data == PacketQueue::sFlushPkt.data) {
                avcodec_flush_buffers(mAvCtx);
                mFinished = false;
                mNextPts = mStartPts;
                mNextPtsTb = mStartPtsTb;
            }
        } while (pkt.data == PacketQueue::sFlushPkt.data ||
                 mPacketQueue->getSerial() != mPktSerial);
        av_packet_unref(&mPkt);
        mLeftoverPkt = mPkt = pkt;
        mPacketPending = true;
    }

    return 0;
}

void Decoder::abort() {
    mPacketQueue->abort();
    mPacketQueue->flush();
}

void Decoder::start() {
    mWorkerThread.start();
}

void Decoder::wait() {
    mWorkerThread.wait();
}

// AudioDecoder implementations

int AudioDecoder::decodeAudioFrame(AVFrame* frame) {
    if (mAvCtx->codec_type != AVMEDIA_TYPE_AUDIO) {
        derror("Codec type: %d is not yet handled", mAvCtx->codec_type);
        return -1;
    }

    AVStream* st = mPlayer->mFormatCtx->streams[mPlayer->mAudioStreamIdx];
    int got_frame = 0;
    do {
        if (mPacketQueue->isAbort()) {
            return -1;
        }

        if (fetchPacket() < 0) {
            return -1;
        }

        int ret = avcodec_decode_audio4(mAvCtx, frame, &got_frame, &mLeftoverPkt);
        if (got_frame) {
            // Try to guess the pts if not set
            if (frame->pts == AV_NOPTS_VALUE && mNextPts != AV_NOPTS_VALUE) {
                frame->pts = mNextPts;
            }
            if (frame->pts != AV_NOPTS_VALUE) {
                mNextPts = frame->pts + av_rescale_q(frame->nb_samples, mAvCtx->time_base, st->time_base);
            }
        }

        if (ret < 0) {
            mPacketPending = false;
        } else {
            mLeftoverPkt.dts = mLeftoverPkt.pts = AV_NOPTS_VALUE;
            if (mLeftoverPkt.data != nullptr) {
                mLeftoverPkt.data += ret;
                mLeftoverPkt.size -= ret;
                if (mLeftoverPkt.size <= 0) {
                    mPacketPending = false;
                }
            } else {
                if (!got_frame) {
                    mPacketPending = false;
                    mFinished = true;
                }
            }
        }
    } while (!got_frame && !mFinished);

    return got_frame;
}

// decode audio frame with a dedicated thread
void AudioDecoder::workerThreadFunc() {
    AVFrame* frame = av_frame_alloc();
    if (frame == nullptr) {
        return;
    }

    while (mPlayer->isRunning()) {
        int got_frame = decodeAudioFrame(frame);
        if (got_frame < 0) {
            break;
        }
        if (got_frame) {
            // Sync to the stream time base, not the codec's time base. We assume that
            // all encoders, if wrapping the data in a container, will set the time base
            // of all of it's containing codecs to the container's timebase.
            AVStream* st = mPlayer->mFormatCtx->streams[mPlayer->mAudioStreamIdx];
            AVRational tb = st->time_base;
            Frame* af = mFrameQueue->peekWritable();
            if (af == nullptr) {
                break;
            }

            af->pts = (frame->pts == AV_NOPTS_VALUE) ? NAN
                                                     : frame->pts * av_q2d(tb);
            af->pos = av_frame_get_pkt_pos(frame);
            af->serial = getPktSerial();
            af->duration =
                    av_q2d((AVRational){frame->nb_samples, frame->sample_rate});
            av_frame_move_ref(af->frame, frame);
            mFrameQueue->push();
        }
    }

    av_frame_free(&frame);
}

// VideoDecoder implementations

int VideoDecoder::decodeVideoFrame(AVFrame* frame) {
    if (mAvCtx->codec_type != AVMEDIA_TYPE_VIDEO) {
        derror("Codec type: %d is not yet handled", mAvCtx->codec_type);
        return -1;
    }

    int got_frame = 0;

    do {
        if (mPacketQueue->isAbort()) {
            return -1;
        }

        if (fetchPacket() < 0) {
            return -1;
        }

        int ret =
                avcodec_decode_video2(mAvCtx, frame, &got_frame, &mLeftoverPkt);
        if (got_frame) {
            if (frame->pkt_pts != AV_NOPTS_VALUE) {
                frame->pts = frame->pkt_pts;
            } else {
                frame->pts = av_frame_get_best_effort_timestamp(frame);
            }
        }

        if (ret < 0) {
            mPacketPending = false;
        } else {
            mLeftoverPkt.dts = mLeftoverPkt.pts = AV_NOPTS_VALUE;
            if (mLeftoverPkt.data != nullptr) {
                mLeftoverPkt.data += ret;
                mLeftoverPkt.size -= ret;
                if (mLeftoverPkt.size <= 0) {
                    mPacketPending = false;
                }
            } else {
                if (!got_frame) {
                    mPacketPending = false;
                    mFinished = true;
                }
            }
        }
    } while (!got_frame && !mFinished);

    return got_frame;
}

int VideoDecoder::getVideoFrame(AVFrame* frame) {
    int got_picture;
    if ((got_picture = decodeVideoFrame(frame)) < 0) {
        return -1;
    }

    if (got_picture) {
        double dpts = NAN;

        AVStream* st = mPlayer->mFormatCtx->streams[mPlayer->mVideoStreamIdx];
        if (frame->pts != AV_NOPTS_VALUE) {
            dpts = av_q2d(st->time_base) * frame->pts;
        }

        frame->sample_aspect_ratio =
                av_guess_sample_aspect_ratio(mPlayer->mFormatCtx.get(), st, frame);

        mPlayer->mWidth = frame->width;
        mPlayer->mHeight = frame->height;

        if ((mPlayer->mFrameDrop > 0 ||
             (mPlayer->mFrameDrop &&
              mPlayer->getMasterSyncType() != AvSyncMaster::Video)) &&
            (frame->pts != AV_NOPTS_VALUE)) {
            double diff = dpts - mPlayer->getMasterClock();
            if (!std::isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD &&
                diff < 0 &&
                getPktSerial() == mPlayer->mVideoClock.getSerial() &&
                mPacketQueue->getNumPackets() > 0) {
                mPlayer->mFrameDropsEarly++;
                av_frame_unref(frame);
                got_picture = 0;
            }
        }
    }

    return got_picture;
}

// decode video frame with a dedicated thread
void VideoDecoder::workerThreadFunc() {
    AVFrame* frame = av_frame_alloc();
    if (frame == nullptr) {
        return;
    }

    AVRational tb =
            mPlayer->mFormatCtx->streams[mPlayer->mVideoStreamIdx]->time_base;
    AVRational frame_rate = av_guess_frame_rate(
            mPlayer->mFormatCtx.get(),
            mPlayer->mFormatCtx->streams[mPlayer->mVideoStreamIdx], NULL);

    while (mPlayer->isRunning()) {
        int ret = getVideoFrame(frame);
        if (ret < 0) {
            break;
        }
        if (!ret) {
            continue;
        }

        double duration =
                (frame_rate.num && frame_rate.den
                         ? av_q2d((AVRational){frame_rate.den, frame_rate.num})
                         : 0);
        double pts =
                (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
        ret = mPlayer->queuePicture(frame, pts, duration,
                                    av_frame_get_pkt_pos(frame),
                                    getPktSerial());
        av_frame_unref(frame);
        if (ret < 0) {
            break;
        }
    }

    av_frame_free(&frame);
}

// VideoPlayer implementations

// create a video player instance the input video file, the output renderTarget to
// display, and the notifier to receive updates
std::unique_ptr<VideoPlayer> VideoPlayer::create(
        std::string videoFile,
        VideoPlayerRenderTarget* renderTarget,
        std::unique_ptr<VideoPlayerNotifier> notifier) {
    std::unique_ptr<VideoPlayerImpl> player;
    player.reset(new VideoPlayerImpl(videoFile, renderTarget, std::move(notifier)));
    return std::move(player);
}

#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define MIN_FRAMES 25
#define EXTERNAL_CLOCK_MIN_FRAMES 2
#define EXTERNAL_CLOCK_MAX_FRAMES 10

// VideoPlayerImpl implementations

VideoPlayerImpl::VideoPlayerImpl(std::string videoFile,
                                 VideoPlayerRenderTarget* renderTarget,
                                 std::unique_ptr<VideoPlayerNotifier> notifier)
    : mVideoFile(videoFile),
      mRenderTarget(renderTarget),
      mNotifier(std::move(notifier)),
      mRunning(true),
      mWorkerThread([this]() { workerThreadFunc(); }) {
    mNotifier->setVideoPlayer(this);
    mNotifier->initTimer();
}

VideoPlayerImpl::~VideoPlayerImpl() {
    stop();
    if (mVideoDecoder) {
        mVideoDecoder->wait();
    }
    if (mAudioDecoder) {
        mAudioDecoder->wait();
    }
    mWorkerThread.wait();
}

// adjust window size to fit the video apect ratio
void VideoPlayerImpl::adjustWindowSize(AVCodecContext* c,
                                       int* pWidth,
                                       int* pHeight) {

    float aspect_ratio;

    if (c->sample_aspect_ratio.num == 0) {
        aspect_ratio = 0;
    } else {
        aspect_ratio = av_q2d(c->sample_aspect_ratio) * c->width / c->height;
    }

    if (aspect_ratio <= 0.0) {
        aspect_ratio = (float)c->width / (float)c->height;
    }

    mRenderTarget->getRenderTargetSize(
        aspect_ratio, c->width, c->height, pWidth, pHeight);
}

AvSyncMaster VideoPlayerImpl::getMasterSyncType() {
    if (this->mAvSyncType == AvSyncMaster::Video) {
        if (mVideoStreamIdx != -1) {
            return AvSyncMaster::Video;
        } else {
            return AvSyncMaster::Audio;
        }
    } else if (this->mAvSyncType == AvSyncMaster::Audio) {
        if (mAudioStreamIdx != -1) {
            return AvSyncMaster::Audio;
        } else {
            return AvSyncMaster::External;
        }
    } else {
        return AvSyncMaster::External;
    }
}

// get the current master clock value
double VideoPlayerImpl::getMasterClock() {
    double val = 0.0;

    switch (getMasterSyncType()) {
        case AvSyncMaster::Video:
            val = this->mVideoClock.getTime();
            break;
        case AvSyncMaster::Audio:
            val = this->mAudioClock.getTime();
            break;
        default:
            val = this->mExternalClock.getTime();
            break;
    }
    return val;
}

void VideoPlayerImpl::checkExternalClockSpeed() {
    if ((mVideoStreamIdx >= 0 &&
         mVideoQueue->getNumPackets() <= EXTERNAL_CLOCK_MIN_FRAMES) ||
        (mAudioStreamIdx >= 0 &&
         mAudioQueue->getNumPackets() <= EXTERNAL_CLOCK_MIN_FRAMES)) {
        mExternalClock.setSpeed(
                FFMAX(EXTERNAL_CLOCK_SPEED_MIN,
                      mExternalClock.getSpeed() - EXTERNAL_CLOCK_SPEED_STEP));
    } else if ((mVideoStreamIdx < 0 ||
                mVideoQueue->getNumPackets() > EXTERNAL_CLOCK_MAX_FRAMES) &&
               (mAudioStreamIdx < 0 ||
                mAudioQueue->getNumPackets() > EXTERNAL_CLOCK_MAX_FRAMES)) {
        mExternalClock.setSpeed(
                FFMIN(EXTERNAL_CLOCK_SPEED_MAX,
                      mExternalClock.getSpeed() + EXTERNAL_CLOCK_SPEED_STEP));
    } else {
        double speed = mExternalClock.getSpeed();
        if (speed != 1.0) {
            mExternalClock.setSpeed(speed + EXTERNAL_CLOCK_SPEED_STEP *
                                                    (1.0 - speed) /
                                                    fabs(1.0 - speed));
        }
    }
}

int VideoPlayerImpl::queuePicture(AVFrame* src_frame,
                                  double pts,
                                  double duration,
                                  int64_t pos,
                                  int serial) {
    Frame* vp = mVideoFrameQueue->peekWritable();
    if (vp == nullptr) {
        return -1;
    }

    vp->sar = src_frame->sample_aspect_ratio;

    // alloc or resize hardware picture buffer
    if (vp->reallocate || !vp->allocated || vp->width != src_frame->width ||
        vp->height != src_frame->height) {
        vp->allocated = false;
        vp->reallocate = false;
        vp->width = mWindowWidth;
        vp->height = mWindowHeight;

        if (vp->buf != nullptr) {
            delete[] vp->buf;
        }

        // Determine required buffer size and allocate buffer
        int numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, mWindowWidth,
                                          mWindowHeight);
        int headerlen = 0;
        vp->buf = new unsigned char[numBytes + 64];
        if (vp->buf != nullptr) {
            // simply append a ppm header to become ppm image format
            headerlen = sprintf((char*)vp->buf, "P6\n%d %d\n255\n",
                                mWindowWidth, mWindowHeight);
        } else {
            headerlen = 0;
        }
        vp->headerlen = headerlen;
        vp->len = numBytes + headerlen;
        VideoPlayerWaitInfo* pwi = mVideoFrameQueue->getWaitInfo();
        pwi->lock.lock();
        vp->allocated = true;
        pwi->done = true;
        pwi->cvDone.signalAndUnlock(&pwi->lock);

        if (mVideoQueue->isAbort()) {
            return -1;
        }
    }

    // assign appropriate parts of buffer to image planes
    AVPicture pict;
    avpicture_fill(&pict, vp->buf + vp->headerlen, AV_PIX_FMT_RGB24,
                   mWindowWidth, mWindowHeight);

    // Convert the image to RGB format
    sws_scale(mImgConvertCtx, src_frame->data, src_frame->linesize, 0,
              mVideoCodecCtx->height, pict.data, pict.linesize);

    vp->pts = pts;
    vp->duration = duration;
    vp->pos = pos;
    vp->serial = serial;

    // now we can update the picture count
    mVideoFrameQueue->push();

    return 0;
}

// render the video
void VideoPlayerImpl::displayVideoFrame(Frame* vp) {
    VideoPlayerRenderTarget::FrameInfo frameInfo;
    frameInfo.headerlen = vp->headerlen;
    frameInfo.width = vp->width;
    frameInfo.height = vp->height;
    mRenderTarget->setPixelBuffer(frameInfo, vp->buf, vp->len);

    mNotifier->emitUpdateWidget();
}

double VideoPlayerImpl::computeTargetDelay(double delay) {
    // update delay to follow master synchronisation source
    if (getMasterSyncType() != AvSyncMaster::Video) {
        // if video is slave, we try to correct big delays by
        // duplicating or deleting a frame
        double diff = mVideoClock.getTime() - getMasterClock();
        D("videoclock=%f masterclock=%f ", mVideoClock.getTime(), getMasterClock());

        // skip or repeat frame. We take into account the
        // delay to compute the threshold. I still don't know
        // if it is the best guess
        double sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN,
                                      FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
        if (!std::isnan(diff) && fabs(diff) < this->mMaxFrameDuration) {
            if (diff <= -sync_threshold) {
                delay = FFMAX(0, delay + diff);
            } else if (diff >= sync_threshold &&
                       delay > AV_SYNC_FRAMEDUP_THRESHOLD) {
                delay = delay + diff;
            } else if (diff >= sync_threshold) {
                delay = 2 * delay;
            }
        }
        D("delay=%f diff=%f sync_threshold=%f\n", delay, diff, sync_threshold);
    }

    return delay;
}

double VideoPlayerImpl::computeVideoFrameDuration(Frame* vp, Frame* nextvp) {
    if (vp->serial == nextvp->serial) {
        double duration = nextvp->pts - vp->pts;
        if (std::isnan(duration) || duration <= 0 ||
            duration > this->mMaxFrameDuration) {
            return vp->duration;
        } else {
            return duration;
        }
    } else {
        return 0.0;
    }
}

void VideoPlayerImpl::updateVideoPts(double pts, int64_t pos, int serial) {
    // update current video pts
    mVideoClock.set(pts, serial);
    mExternalClock.syncToSlave(&mVideoClock);
}

// rdft speed in secs
double rdftspeed = 0.02;

// called to display each frame
void VideoPlayerImpl::refreshFrame(double* remaining_time) {
    if (!mPaused && getMasterSyncType() == AvSyncMaster::External &&
        this->mRealTime) {
        checkExternalClockSpeed();
    }

    if (mAudioStreamIdx != -1) {
        double time = av_gettime_relative() / 1000000.0;
        if (this->mForceRefresh || this->mLastVisibleTime + rdftspeed < time) {
            displayVideoFrame(mVideoFrameQueue->peek());
            this->mLastVisibleTime = time;
        }
        *remaining_time = FFMIN(*remaining_time,
                                this->mLastVisibleTime + rdftspeed - time);
    }

    if (mVideoStreamIdx == -1) {
        return;
    }

    int redisplay = 0;
    if (this->mForceRefresh) {
        redisplay = mVideoFrameQueue->prev();
    }

retry:
    if (mVideoFrameQueue->numRemaining() == 0) {
        // nothing to do, no picture to display in the queue
    } else {
        // dequeue the picture
        Frame* lastvp = mVideoFrameQueue->peekLast();
        Frame* vp = mVideoFrameQueue->peek();
        if (vp->serial != mVideoQueue->getSerial()) {
            mVideoFrameQueue->next();
            redisplay = 0;
            goto retry;
        }

        if (lastvp->serial != vp->serial && !redisplay) {
            this->mFrameTimer = av_gettime_relative() / 1000000.0;
        }

        if (!mPaused) {
            double delay;
            // compute nominal last_duration
            double last_duration = computeVideoFrameDuration(lastvp, vp);
            if (redisplay) {
                delay = 0.0;
            } else {
                delay = computeTargetDelay(last_duration);
            }

            double time = av_gettime_relative() / 1000000.0;
            if (time < this->mFrameTimer + delay && !redisplay) {
                *remaining_time = FFMIN(this->mFrameTimer + delay - time,
                                        *remaining_time);
                return;
            }

            this->mFrameTimer += delay;
            if (delay > 0 && time - this->mFrameTimer > AV_SYNC_THRESHOLD_MAX) {
                this->mFrameTimer = time;
            }

            VideoPlayerWaitInfo* pwi = mVideoFrameQueue->getWaitInfo();
            pwi->lock.lock();
            if (!redisplay && !std::isnan(vp->pts)) {
                updateVideoPts(vp->pts, vp->pos, vp->serial);
            }
            pwi->lock.unlock();

            if (mVideoFrameQueue->numRemaining() > 1) {
                Frame* nextvp = mVideoFrameQueue->peekNext();
                double duration = computeVideoFrameDuration(vp, nextvp);
                if ((redisplay || mFrameDrop > 0 ||
                     (mFrameDrop &&
                      getMasterSyncType() != AvSyncMaster::Video)) &&
                    time > this->mFrameTimer + duration) {
                    if (!redisplay) {
                        this->mFrameDropsLate++;
                    }
                    mVideoFrameQueue->next();
                    redisplay = 0;
                    goto retry;
                }
            }
        }

        // display picture */
        displayVideoFrame(mVideoFrameQueue->peek());

        mVideoFrameQueue->next();
    }

    this->mForceRefresh = false;
}

// possible required screen refresh at least this often, should be less than
// 1/fps, in seconds, following corresponds to 100 fps
#define REFRESH_DURATION 0.01

void VideoPlayerImpl::videoRefresh() {
    if (!mRunning && mVideoFrameQueue.get() != nullptr &&
        mVideoFrameQueue->size() == 0) {
        if (mNotifier != nullptr) {
            mNotifier->stopTimer();
        }
        return;
    }

    if (mVideoFrameQueue.get() == nullptr) {
        scheduleRefresh(20);
        return;
    }

    double remaining_time = REFRESH_DURATION;
    if (!this->mPaused || this->mForceRefresh) {
        refreshFrame(&remaining_time);
    }

    scheduleRefresh(remaining_time * 1000);
}

// schedule a timer to start in the specified milliseconds
void VideoPlayerImpl::scheduleRefresh(int delayMs) {
    if (mNotifier != nullptr) {
        if (mRunning) {
            mNotifier->startTimer(delayMs);
        } else {
            mNotifier->stopTimer();
        }
    }
}

bool VideoPlayerImpl::isRealTimeFormat(AVFormatContext* s) {
    if (!strcmp(s->iformat->name, "rtp") || !strcmp(s->iformat->name, "rtsp") ||
        !strcmp(s->iformat->name, "sdp")) {
        return true;
    }

    if (s->pb && (!strncmp(s->filename, "rtp:", 4) ||
                  !strncmp(s->filename, "udp:", 4))) {
        return true;
    }

    return false;
}

int VideoPlayerImpl::play() {
    const char* filename = mVideoFile.c_str();

    // Register all formats and codecs
    av_register_all();
    avformat_network_init();

    PacketQueue::init();

    AVFormatContext* inputCtx;
    if (avformat_open_input(&inputCtx, filename, NULL, NULL) != 0) {
        LOG(ERROR) << __func__ << ": Failed to open input context";
        return -1;
    }
    mFormatCtx = makeAVScopedPtr(inputCtx);

    if (avformat_find_stream_info(mFormatCtx.get(), NULL) < 0) {
        LOG(ERROR) << __func__ << ": Failed to find stream info";
        return -1;
    }

    this->mMaxFrameDuration =
            (mFormatCtx->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;

    this->mRealTime = isRealTimeFormat(mFormatCtx.get());

    // dump video format
    av_dump_format(mFormatCtx.get(), 0, filename, false);

    mRunning = true;

    // Find the first audio/video stream
    int audioStream = -1;
    int videoStream = -1;
    for (int i = 0; i < mFormatCtx->nb_streams; i++) {
        if (mFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStream = i;
            if (videoStream != -1) {
                break;
            }
        } else if (mFormatCtx->streams[i]->codec->codec_type ==
                   AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            if (audioStream != -1) {
                break;
            }
        }
    }

    if (videoStream == -1 && audioStream == -1) {
        LOG(ERROR) << __func__ << ": No audio or video stream found";
        return -1;
    }

    // we do this before mAudioClock since audio callback uses it
    mExternalClock.init(mExternalClock.getSerialPtr());

    if (audioStream != -1) {
        mAudioOutputEngine = AudioOutputEngine::get();

        mAudioStreamIdx = audioStream;

        // Find the decoder for the video stream
        mAudioCodec = avcodec_find_decoder(mFormatCtx->streams[audioStream]->codec->codec_id);
        if (mAudioCodec == nullptr) {
            return -1;
        }

        /* Allocate a codec context for the decoder */
        mAudioCodecCtx = makeAVScopedPtr(avcodec_alloc_context3(mAudioCodec));
        if (!mAudioCodecCtx) {
            LOG(ERROR) << __func__ << ": Failed to allocate audio codec context";
            return -1;
        }

        /* Copy codec parameters from input stream to output codec context */
        if (avcodec_parameters_to_context(
            mAudioCodecCtx.get(), mFormatCtx->streams[audioStream]->codecpar) < 0) {
            LOG(ERROR) << __func__ << ": Failed to copy audio codec parameters to decoder context";
            return -1;
        }

        // Open codec
        if (avcodec_open2(mAudioCodecCtx.get(), mAudioCodec, NULL) < 0) {
            LOG(ERROR) << __func__ << ": Failed to open audio codec";
            return -1;
        }

        if (mAudioOutputEngine != nullptr) {
            mAudioOutputEngine->open(
                    "video-player", android::emulation::AudioFormat::S16,
                    mAudioCodecCtx->sample_rate, mAudioCodecCtx->channels,
                    audioCallback, this);
        }

        // create resampler context
        mSwrCtx = swr_alloc();
        if (mSwrCtx == nullptr) {
            derror("Could not allocate resampler context");
            return -1;
        }

        // set options
        av_opt_set_int(mSwrCtx, "in_channel_count", mAudioCodecCtx->channels,
                       0);
        av_opt_set_int(mSwrCtx, "in_sample_rate", mAudioCodecCtx->sample_rate,
                       0);
        av_opt_set_sample_fmt(mSwrCtx, "in_sample_fmt",
                              mAudioCodecCtx->sample_fmt, 0);
        av_opt_set_int(mSwrCtx, "out_channel_count", mAudioCodecCtx->channels,
                       0);
        av_opt_set_int(mSwrCtx, "out_sample_rate", mAudioCodecCtx->sample_rate,
                       0);
        av_opt_set_sample_fmt(mSwrCtx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

        // initialize the resampling context
        if (swr_init(mSwrCtx) < 0) {
            derror("Failed to initialize the resampling context");
            return -1;
        }

        mAudioQueue.reset(new PacketQueue(this));
        mAudioQueue->start();

        mAudioClock.init(mAudioQueue->getSerialPtr());

        mAudioFrameQueue.reset(new FrameQueue(this, mAudioQueue.get(),
                                              AUDIO_SAMPLE_QUEUE_SIZE, true));

        mAudioDecoder.reset(new AudioDecoder(
                this, mAudioCodecCtx.get(), mAudioQueue.get(), mAudioFrameQueue.get(),
                &mContinueReadWaitInfo));
        mAudioDecoder->start();
    }

    if (videoStream != -1) {
        mVideoStreamIdx = videoStream;


        // Find the decoder for the video stream
        mVideoCodec = avcodec_find_decoder(mFormatCtx->streams[videoStream]->codec->codec_id);
        if (mVideoCodec == nullptr) {
            return -1;
        }

        /* Allocate a codec context for the decoder */
        mVideoCodecCtx = makeAVScopedPtr(avcodec_alloc_context3(mVideoCodec));
        if (!mVideoCodecCtx) {
            LOG(ERROR) << __func__ << ": Failed to copy video codec parameters to decoder context";
            return -1;
        }

        /* Copy codec parameters from input stream to output codec context */
        if (avcodec_parameters_to_context(
            mVideoCodecCtx.get(), mFormatCtx->streams[videoStream]->codecpar) < 0) {
            LOG(ERROR) << __func__ << ": Failed to copy video codec parameters to decoder context";
            return -1;
        }

        // Open codec
        if (avcodec_open2(mVideoCodecCtx.get(), mVideoCodec, NULL) < 0) {
            LOG(ERROR) << __func__ << ": Failed to video context";
            return -1;
        }

        mWidth = mVideoCodecCtx->width;
        mHeight = mVideoCodecCtx->height;

        int dst_w = mVideoCodecCtx->width;
        int dst_h = mVideoCodecCtx->height;
        adjustWindowSize(mVideoCodecCtx.get(), &dst_w, &dst_h);

        // window size to display the video
        mWindowWidth = dst_w;
        mWindowHeight = dst_h;

        AVPixelFormat dst_fmt = AV_PIX_FMT_RGB24;
        // create image convert context
        mImgConvertCtx =
                sws_getContext(mVideoCodecCtx->width, mVideoCodecCtx->height,
                               mVideoCodecCtx->pix_fmt, dst_w, dst_h, dst_fmt,
                               SWS_FAST_BILINEAR, NULL, NULL, NULL);
        if (mImgConvertCtx == nullptr) {
            derror("Could not allocate image convert context");
            return -1;
        }

        mVideoQueue.reset(new PacketQueue(this));
        mVideoQueue->start();

        mVideoClock.init(mVideoQueue->getSerialPtr());

        mVideoFrameQueue.reset(new FrameQueue(this, mVideoQueue.get(),
                                              VIDEO_PICTURE_QUEUE_SIZE, true));

        mVideoDecoder.reset(new VideoDecoder(
                this, mVideoCodecCtx.get(), mVideoQueue.get(), mVideoFrameQueue.get(),
                &mContinueReadWaitInfo));
        mVideoDecoder->start();
    }

    // Read frames from the video file
    int ret = 0;
    AVPacket packet;
    while (mRunning && (ret = av_read_frame(mFormatCtx.get(), &packet)) >= 0) {
        if (packet.stream_index == audioStream) {
            mAudioQueue->put(&packet);
        } else if (packet.stream_index == videoStream) {
            mVideoQueue->put(&packet);
        } else {
            // stream that we don't handle, simply free and ignore it
            av_free_packet(&packet);
        }

        // if the queues are full, no need to read more
        int curent_queues_total_size = 0;
        if (mAudioQueue.get() != nullptr) {
            curent_queues_total_size += mAudioQueue->getSize();
        }
        if (mVideoQueue.get() != nullptr) {
            curent_queues_total_size += mVideoQueue->getSize();
        }

        if (curent_queues_total_size > MAX_QUEUE_SIZE ||
            (mFormatCtx->streams[mVideoStreamIdx]->disposition &
             AV_DISPOSITION_ATTACHED_PIC)) {
            // wait 10 ms
            android::base::System::Duration timeoutMs = 10ll;
            VideoPlayerWaitInfo* pwi = &mContinueReadWaitInfo;
            pwi->lock.lock();
            const auto deadlineUs =
                    android::base::System::get()->getUnixTimeUs() +
                    timeoutMs * 1000;
            while (android::base::System::get()->getUnixTimeUs() < deadlineUs) {
                pwi->cvDone.timedWait(&pwi->lock, deadlineUs);
            }
            pwi->lock.unlock();
        }
    }

    if (ret == AVERROR_EOF || avio_feof(mFormatCtx->pb)) {
        if (videoStream >= 0) {
            mVideoQueue->putNullPacket(videoStream);
        }

        if (audioStream >= 0) {
            mAudioQueue->putNullPacket(audioStream);
        }
    }

    if (mAudioDecoder.get() != nullptr) {
        mAudioDecoder->wait();
        mAudioDecoder.reset();
    }

    if (mVideoDecoder.get() != nullptr) {
        mVideoDecoder->wait();
        mVideoDecoder.reset();
    }

    const bool wasRunning = mRunning;
    mRunning = false;

    cleanup();

    if (wasRunning) {
        mNotifier->emitVideoStopped();
    }
    mNotifier->emitVideoFinished();

    return 0;
}

void VideoPlayerImpl::workerThreadFunc() {
    int rc = play();
    (void)rc;
}

// get an audio frame from the decoded queue, and convert it to buffer
int VideoPlayerImpl::getConvertedAudioFrame() {
    if (mAudioFrameQueue.get() == nullptr) {
        return -1;
    }

    Frame* af = mAudioFrameQueue->peekReadableNoWait();
    if (af == nullptr) {
        return -1;
    }

    mAudioFrameQueue->next();

    int data_size = av_samples_get_buffer_size(
            NULL, av_frame_get_channels(af->frame), af->frame->nb_samples,
            (enum AVSampleFormat)af->frame->format, 1);
    int wanted_nb_samples = af->frame->nb_samples;

    int resampled_data_size;

    if (this->mSwrCtx) {
        const uint8_t** in = (const uint8_t**)af->frame->extended_data;
        uint8_t** out = &this->mAudioBuf1;
        int out_count = (int64_t)wanted_nb_samples + 256;
        int out_size = av_samples_get_buffer_size(NULL, 2, out_count,
                                                  AV_SAMPLE_FMT_S16, 0);
        if (out_size < 0) {
            return -1;
        }
        av_fast_malloc(&this->mAudioBuf1, &this->mAudioBuf1Size, out_size);
        if (!this->mAudioBuf1) {
            return AVERROR(ENOMEM);
        }
        int len2 = swr_convert(this->mSwrCtx, out, out_count, in,
                               af->frame->nb_samples);
        if (len2 < 0) {
            return -1;
        }
        if (len2 == out_count) {
            if (swr_init(this->mSwrCtx) < 0)
                swr_free(&this->mSwrCtx);
        }
        this->mAudioBuf = this->mAudioBuf1;
        resampled_data_size = len2 * 2 * 2;
    } else {
        this->mAudioBuf = af->frame->data[0];
        resampled_data_size = data_size;
    }

    // update the audio clock with the pts
    if (!std::isnan(af->pts)) {
        this->mAudioClockValue = af->pts + (double)af->frame->nb_samples /
                                                   af->frame->sample_rate;
        D("pts=%f nb_samples=%d sample_rate=%d\n", af->pts, af->frame->nb_samples, af->frame->sample_rate);
    } else {
        this->mAudioClockValue = NAN;
    }

    this->mAudioClockSerial = af->serial;

    return resampled_data_size;
}

// we can't block here, otherwise cause the whole emulator to freeze
void VideoPlayerImpl::audioCallback(void* opaque, int len) {
    VideoPlayerImpl* pThis = (VideoPlayerImpl*)opaque;
    pThis->mAudioCallbackTime = av_gettime_relative();
    while (len > 0) {
        if (pThis->mAudioBufIndex >= pThis->mAudioBufSize) {
            int audio_size = pThis->getConvertedAudioFrame();
            if (audio_size < 0) {
                // if error, we output silent buffer
                std::string silent(len, '\0');
                pThis->mAudioOutputEngine->write((void *)silent.data(), len);
                break;
            } else {
                pThis->mAudioBufSize = audio_size;
            }
            pThis->mAudioBufIndex = 0;
        }

        int len1 = pThis->mAudioBufSize - pThis->mAudioBufIndex;
        if (len1 > len) {
            len1 = len;
        }

        pThis->mAudioOutputEngine->write(
                pThis->mAudioBuf + pThis->mAudioBufIndex, len1);
        len -= len1;
        pThis->mAudioBufIndex += len1;
    }

    pThis->mAudioWriteBufSize = pThis->mAudioBufSize - pThis->mAudioBufIndex;

    if (!std::isnan(pThis->mAudioClockValue)) {
        int bytes_per_sec = av_samples_get_buffer_size(NULL, 2, 48000, AV_SAMPLE_FMT_S16, 1);
        pThis->mAudioClock.setAt(pThis->mAudioClockValue - (double)(2 * 1024 + pThis->mAudioWriteBufSize) / bytes_per_sec, pThis->mAudioClockSerial, pThis->mAudioCallbackTime / 1000000.0);
        pThis->mExternalClock.syncToSlave(&pThis->mAudioClock);
        D("mAudioclock=%f maudioclockvalue=%f writebufsiize=%d\n", pThis->mAudioClock.getTime(), pThis->mAudioClockValue, pThis->mAudioWriteBufSize);
    }
}

void VideoPlayerImpl::start() {
    mWorkerThread.start();
}

void VideoPlayerImpl::stop() {
    mRunning = false;

    mNotifier->stopTimer();

    internalStop();
}

void VideoPlayerImpl::internalStop() {
    mRunning = false;

    if (mAudioDecoder.get() != nullptr) {
        mAudioDecoder->abort();
    }

    if (mVideoDecoder.get() != nullptr) {
        mVideoDecoder->abort();
    }

    if (mAudioQueue.get() != nullptr) {
        mAudioQueue->signalWait();
        // mAudioQueue.reset();
    }

    if (mVideoQueue.get() != nullptr) {
        mVideoQueue->signalWait();
        // mVideoQueue.reset();
    }

    if (mAudioFrameQueue.get() != nullptr) {
        mAudioFrameQueue->signalWait();
        // mAudioFrameQueue.reset();
    }

    if (mVideoFrameQueue.get() != nullptr) {
        mVideoFrameQueue->signalWait();
        // mVideoFrameQueue.reset();
    }
}

void VideoPlayerImpl::cleanup() {
    if (mAudioDecoder.get() != nullptr) {
        mAudioDecoder->abort();
    }

    if (mVideoDecoder.get() != nullptr) {
        mVideoDecoder->abort();
    }

    if (mAudioFrameQueue.get() != nullptr) {
        mAudioFrameQueue->signalWait();
        // mAudioFrameQueue.reset();
    }

    if (mVideoFrameQueue.get() != nullptr) {
        mVideoFrameQueue->signalWait();
        // mVideoFrameQueue.reset();
    }

    VideoPlayerRenderTarget::FrameInfo dummyInfo;
    mRenderTarget->setPixelBuffer(dummyInfo, nullptr, 0);

    if (mImgConvertCtx != nullptr) {
        sws_freeContext(mImgConvertCtx);
        mImgConvertCtx = nullptr;
    }

    if (mSwrCtx != nullptr) {
        swr_free(&mSwrCtx);
        mSwrCtx = nullptr;
    }

    mAudioCodecCtx.reset();
    mVideoCodecCtx.reset();

    // close the video file
    mFormatCtx.reset();

    if (mAudioOutputEngine != nullptr) {
        mAudioOutputEngine->close();
        mAudioOutputEngine = nullptr;
    }
}

}  // namespace videoplayer
}  // namespace android
