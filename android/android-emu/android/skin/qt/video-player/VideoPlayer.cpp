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

#include "android/skin/qt/video-player/VideoPlayer.h"
#include "android/utils/debug.h"
#include "android/base/threads/Thread.h"

namespace android {
namespace videoplayer {

#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define MIN_FRAMES 25
#define EXTERNAL_CLOCK_MIN_FRAMES 2
#define EXTERNAL_CLOCK_MAX_FRAMES 10

//// VideoPlayer implementations

VideoPlayer::VideoPlayer(VideoPlayerWidget* widget, std::string videoFile)
    : mVideoFile(videoFile), mWidget(widget), mRunning(true) {
    mTimer.setSingleShot(true);
    QObject::connect(&mTimer, &QTimer::timeout, this,
                     &VideoPlayer::videoRefreshTimer);
}

void VideoPlayer::run() {
    play(mVideoFile.c_str());
}

// adjust window size to fit the video apect ratio
void VideoPlayer::adjustWindowSize(AVCodecContext* c,
                                   int* pWidth,
                                   int* pHeight) {
    float aspect_ratio;

    if (c->sample_aspect_ratio.num == 0) {
        aspect_ratio = 0;
    } else {
        aspect_ratio = av_q2d(c->sample_aspect_ratio) *
                       c->width / c->height;
    }

    if (aspect_ratio <= 0.0) {
        aspect_ratio = (float)c->width / (float)c->height;
    }

    int h = mWidget->height();
    int w = ((int)(h * aspect_ratio)) & -3;
    if (w > mWidget->width()) {
        w = mWidget->width();
        h = ((int)(w / aspect_ratio)) & -3;
    }

    int x = (mWidget->width() - w) / 2;
    int y = (mWidget->height() - h) / 2;

    if (mWidget->width() != w || mWidget->height() != h) {
        mWidget->move(x, y);
        mWidget->setFixedSize(w, h);
    }

    *pWidth = w;
    *pHeight = h;
}

// decode audio frame with a dedicated thread
void VideoPlayer::decodeAudioThread()
{
    AVFrame* frame = av_frame_alloc();
    if (frame == nullptr) {
        return;
    }

    while (mRunning) {
        int got_frame = mAudioDecoder->decodeFrame(frame);
        if (got_frame < 0) {
            break;
        }
        if (got_frame) {
            AVRational tb = (AVRational){1, frame->sample_rate};
            Frame* af = mAudioFrameQueue->peekWritable();
            if (af == nullptr) {
                break;
            }

            af->pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
            af->pos = av_frame_get_pkt_pos(frame);
            af->serial = mAudioDecoder->getPktSerial();
            af->duration = av_q2d((AVRational){frame->nb_samples, frame->sample_rate});
            av_frame_move_ref(af->frame, frame);
            mAudioFrameQueue->push();
        }
    }

    av_frame_free(&frame);
}

int VideoPlayer::getVideoFrame(AVFrame *frame)
{
    int got_picture;
    if ((got_picture = mVideoDecoder->decodeFrame(frame)) < 0) {
        return -1;
    }

    if (got_picture) {
        double dpts = NAN;

        AVStream* st = mFormatCtx->streams[mVideoStreamIdx];
        if (frame->pts != AV_NOPTS_VALUE) {
            dpts = av_q2d(st->time_base) * frame->pts;
        }

        frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(mFormatCtx, st, frame);

        mWidth  = frame->width;
        mHeight = frame->height;

        if (mFrameDrop > 0 || (mFrameDrop && getMasterSyncType() != AvSyncMaster::Video)) {
            if (frame->pts != AV_NOPTS_VALUE) {
                double diff = dpts - getMasterClock();
                if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD &&
                    diff < 0 &&
                    mVideoDecoder->getPktSerial() == mVideoClock.getSerial() &&
                    mVideoQueue->getNumPackets() > 0) {
                    this->mFrameDropsEarly ++;
                    av_frame_unref(frame);
                    got_picture = 0;
                }
            }
        }
    }

    return got_picture;
}

// decode video frame with a dedicated thread
void VideoPlayer::decodeVideoThread()
{
    AVFrame *frame = av_frame_alloc();
    if (frame == nullptr) {
        return;
    }

    AVRational tb = mFormatCtx->streams[mVideoStreamIdx]->time_base;
    AVRational frame_rate = av_guess_frame_rate(mFormatCtx, mFormatCtx->streams[mVideoStreamIdx], NULL);

    while (mRunning) {
        int ret = getVideoFrame(frame);
        if (ret < 0) {
            break;
        }
        if (!ret) {
            continue;
        }

        double duration = (frame_rate.num && frame_rate.den ? av_q2d((AVRational){frame_rate.den, frame_rate.num}) : 0);
        double pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
        ret = queuePicture(frame, pts, duration, av_frame_get_pkt_pos(frame), mVideoDecoder->getPktSerial());
        av_frame_unref(frame);
        if (ret < 0) {
            break;
        }
    }

    av_frame_free(&frame);
}

AvSyncMaster VideoPlayer::getMasterSyncType()
{
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
double VideoPlayer::getMasterClock()
{
    double val = 0.0;

    switch (getMasterSyncType()) {
        case AvSyncMaster::Video:
            val = this->mVideoClock.get();
            break;
        case AvSyncMaster::Audio:
            val = this->mAudioClock.get();
            break;
        default:
            val = this->mExternalClock.get();
            break;
    }
    return val;
}

void VideoPlayer::checkExternalClockSpeed()
{
    if ((mVideoStreamIdx >= 0 && mVideoQueue->getNumPackets() <= EXTERNAL_CLOCK_MIN_FRAMES) ||
        (mAudioStreamIdx >= 0 && mAudioQueue->getNumPackets() <= EXTERNAL_CLOCK_MIN_FRAMES)) {
        mExternalClock.setSpeed(FFMAX(EXTERNAL_CLOCK_SPEED_MIN, mExternalClock.getSpeed() - EXTERNAL_CLOCK_SPEED_STEP));
    } else if ((mVideoStreamIdx< 0 || mVideoQueue->getNumPackets() > EXTERNAL_CLOCK_MAX_FRAMES) &&
               (mAudioStreamIdx < 0 || mAudioQueue->getNumPackets() > EXTERNAL_CLOCK_MAX_FRAMES)) {
        mExternalClock.setSpeed(FFMIN(EXTERNAL_CLOCK_SPEED_MAX, mExternalClock.getSpeed() + EXTERNAL_CLOCK_SPEED_STEP));
    } else {
        double speed = mExternalClock.getSpeed();
        if (speed != 1.0) {
            mExternalClock.setSpeed(speed + EXTERNAL_CLOCK_SPEED_STEP * (1.0 - speed) / fabs(1.0 - speed));
        }
    }
}

int VideoPlayer::queuePicture(AVFrame *src_frame, double pts, double duration, int64_t pos, int serial)
{
    Frame* vp = mVideoFrameQueue->peekWritable();
    if (vp == nullptr) {
        return -1;
    }

    vp->sar = src_frame->sample_aspect_ratio;

    // alloc or resize hardware picture buffer
    if (vp->reallocate ||
        !vp->allocated ||
        vp->width != src_frame->width ||
        vp->height != src_frame->height) {
        vp->allocated  = false;
        vp->reallocate = false;
        vp->width = src_frame->width;
        vp->height = src_frame->height;

        if (vp->buf != nullptr) {
            delete [] vp->buf;
        }

        // Determine required buffer size and allocate buffer
        int numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, mWindowWidth, mWindowHeight);
        int headerlen = 0;
        vp->buf = new unsigned char[numBytes + 64];
        if (vp->buf != nullptr) {
            // simply append a ppm header to become ppm image format
            headerlen = sprintf((char*)vp->buf, "P6\n%d %d\n255\n", mWindowWidth, mWindowHeight);
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
void VideoPlayer::displayVideoFrame(Frame* vp) {
    mWidget->setPixelBuffer(vp->buf, vp->len);

    emit updateWidget();
}

double VideoPlayer::computeTargetDelay(double delay)
{
    // update delay to follow master synchronisation source
    if (getMasterSyncType() != AvSyncMaster::Video) {
        // if video is slave, we try to correct big delays by
        // duplicating or deleting a frame
        double diff = mVideoClock.get() - getMasterClock();

        // skip or repeat frame. We take into account the
        // delay to compute the threshold. I still don't know
        // if it is the best guess
        double sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
        if (!isnan(diff) && fabs(diff) < this->mMaxFrameDuration) {
            if (diff <= -sync_threshold) {
                delay = FFMAX(0, delay + diff);
            } else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD) {
                delay = delay + diff;
            } else if (diff >= sync_threshold) {
                delay = 2 * delay;
            }
        }
    }

    return delay;
}

double VideoPlayer::computeVideoFrameDuration(Frame *vp, Frame *nextvp)
{
    if (vp->serial == nextvp->serial) {
        double duration = nextvp->pts - vp->pts;
        if (isnan(duration) || duration <= 0 || duration > this->mMaxFrameDuration) {
            return vp->duration;
        } else {
            return duration;
        }
    } else {
       return 0.0;
    }
}

void VideoPlayer::updateVideoPts(double pts, int64_t pos, int serial)
{
    // update current video pts
    mVideoClock.set(pts, serial);
    mExternalClock.syncToSlave(&mVideoClock);
}

// rdft speed in msecs
double rdftspeed = 0.02;

// called to display each frame
void VideoPlayer::videoRefresh(double *remaining_time)
{
    if (!mPaused && getMasterSyncType() == AvSyncMaster::External && this->mRealTime) {
        checkExternalClockSpeed();
    }

    if (mAudioStreamIdx != -1) {
        double time = av_gettime_relative() / 1000000.0;
        if (this->mForceRefresh || this->mLastVisibleTime + rdftspeed < time) {
            displayVideoFrame(mVideoFrameQueue->peek());
            this->mLastVisibleTime = time;
        }
        *remaining_time = FFMIN(*remaining_time, this->mLastVisibleTime + rdftspeed - time);
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

            double time= av_gettime_relative() / 1000000.0;
            if (time < this->mFrameTimer + delay && !redisplay) {
                *remaining_time = FFMIN(this->mFrameTimer + delay - time, *remaining_time);
                return;
            }

            this->mFrameTimer += delay;
            if (delay > 0 && time - this->mFrameTimer > AV_SYNC_THRESHOLD_MAX) {
                this->mFrameTimer = time;
            }

            VideoPlayerWaitInfo* pwi = mVideoFrameQueue->getWaitInfo();
            pwi->lock.lock();
            if (!redisplay && !isnan(vp->pts)) {
                updateVideoPts(vp->pts, vp->pos, vp->serial);
            }
            pwi->lock.unlock();

            if (mVideoFrameQueue->numRemaining() > 1) {
                Frame* nextvp = mVideoFrameQueue->peekNext();
                double  duration = computeVideoFrameDuration(vp, nextvp);
                if ((redisplay || mFrameDrop > 0 || (mFrameDrop && getMasterSyncType() != AvSyncMaster::Video)) && time > this->mFrameTimer + duration) {
                    if (!redisplay) {
                        this->mFrameDropsLate ++;
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

// possible required screen refresh at least this often, should be less than 1/fps
#define REFRESH_RATE 0.01

void VideoPlayer::videoRefreshTimer() {
    if (!mRunning && mVideoFrameQueue->size() == 0) {
        mTimer.stop();
        return;
    }

    double remaining_time = REFRESH_RATE;
    if (!this->mPaused || this->mForceRefresh) {
        videoRefresh(&remaining_time);
    }

    scheduleRefresh(remaining_time * 1000);
}

// schedule a timer to start in the specified milliseconds
void VideoPlayer::scheduleRefresh(int delayMs) {
    if (mRunning) {
        mTimer.start(delayMs);
    } else {
        mTimer.stop();
    }
}

bool VideoPlayer::isRealTimeFormat(AVFormatContext *s)
{
    if(   !strcmp(s->iformat->name, "rtp")
       || !strcmp(s->iformat->name, "rtsp")
       || !strcmp(s->iformat->name, "sdp")
       ) {
        return true;
    }

    if (s->pb && (  !strncmp(s->filename, "rtp:", 4)
                 || !strncmp(s->filename, "udp:", 4)
                  )) {
        return true;
    }

    return false;
}

int VideoPlayer::play(const char* filename) {
    // Register all formats and codecs
    av_register_all();
    avformat_network_init();

    PacketQueue::init();

    if (avformat_open_input(&mFormatCtx, filename, NULL, NULL) != 0) {
        return -1;  // failed to open video file
    }

    if (avformat_find_stream_info(mFormatCtx, NULL) < 0) {
        return -1;  // failed to find stream info
    }

    this->mMaxFrameDuration = (mFormatCtx->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;

    this->mRealTime = isRealTimeFormat(mFormatCtx);

    // dump video format
    av_dump_format(mFormatCtx, 0, filename, false);

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
        return -1;  // no audio or video stream found
    }

    mAudioOutputEngine = AudioOutputEngine::get();

    if (audioStream != -1) {
        mAudioStreamIdx = audioStream;
        mAudioCodecCtx = mFormatCtx->streams[audioStream]->codec;

        // Find the decoder for the video stream
        mAudioCodec = avcodec_find_decoder(mAudioCodecCtx->codec_id);
        if (mAudioCodec == nullptr) {
            return -1;
        }

        // Open codec
        if (avcodec_open2(mAudioCodecCtx, mAudioCodec, NULL) < 0) {
            return -1;
        }

        if (mAudioOutputEngine != nullptr) {
            mAudioOutputEngine->open("video-player",
                android::emulation::AudioFormat::S16, mAudioCodecCtx->sample_rate,
                mAudioCodecCtx->channels, audioCallback, this);
        }

        // create resampler context
        mSwrCtx = swr_alloc();
        if (mSwrCtx == nullptr) {
            derror("Could not allocate resampler context");
            return -1;
        }

        // set options
        av_opt_set_int(mSwrCtx, "in_channel_count", mAudioCodecCtx->channels, 0);
        av_opt_set_int(mSwrCtx, "in_sample_rate", mAudioCodecCtx->sample_rate, 0);
        av_opt_set_sample_fmt(mSwrCtx, "in_sample_fmt", mAudioCodecCtx->sample_fmt, 0);
        av_opt_set_int(mSwrCtx, "out_channel_count", mAudioCodecCtx->channels, 0);
        av_opt_set_int(mSwrCtx, "out_sample_rate", mAudioCodecCtx->sample_rate, 0);
        av_opt_set_sample_fmt(mSwrCtx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

        // initialize the resampling context
        if (swr_init(mSwrCtx) < 0) {
            derror("Failed to initialize the resampling context");
            return -1;
        }

        mAudioQueue.reset(new PacketQueue(this));
        mAudioQueue->start();

        mAudioClock.init(mAudioQueue->getSerialPtr());

        mAudioFrameQueue.reset(new FrameQueue(this, mAudioQueue.get(), AUDIO_SAMPLE_QUEUE_SIZE, true));

        mAudioDecoder.reset(new Decoder(this, mAudioCodecCtx, mAudioQueue.get(), &mContinueReadWaitInfo));
        //mAudioDecoder->startThread(ThreadType::AudioDecoding);
        mAudioDecodingThread.reset(new ThreadWrapper(this, ThreadType::AudioDecoding));
        mAudioDecodingThread->start();
    }

    if (videoStream != -1) {
        mVideoStreamIdx = videoStream;

        mVideoCodecCtx = mFormatCtx->streams[videoStream]->codec;

        // Find the decoder for the video stream
        mVideoCodec = avcodec_find_decoder(mVideoCodecCtx->codec_id);
        if (mVideoCodec == nullptr) {
            return -1;
        }

        // Open codec
        if (avcodec_open2(mVideoCodecCtx, mVideoCodec, NULL) < 0) {
            return -1;
        }

        mWidth = mVideoCodecCtx->width;
        mHeight = mVideoCodecCtx->height;

        int dst_w = mVideoCodecCtx->width;
        int dst_h = mVideoCodecCtx->height;
        adjustWindowSize(mVideoCodecCtx, &dst_w, &dst_h);

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

        mVideoFrameQueue.reset(new FrameQueue(this, mVideoQueue.get(), VIDEO_PICTURE_QUEUE_SIZE, true));

        mVideoDecoder.reset(new Decoder(this, mVideoCodecCtx, mVideoQueue.get(), &mContinueReadWaitInfo));
        //mVideoDecoder->startThread(ThreadType::VideoDecoding); // weird, this does not work well
        mVideoDecodingThread.reset(new ThreadWrapper(this, ThreadType::VideoDecoding));
        mVideoDecodingThread->start();
    }

    mExternalClock.init(mExternalClock.getSerialPtr());

    // Read frames from the video file
    int ret = 0;
    AVPacket packet;
    while (mRunning && (ret = av_read_frame(mFormatCtx, &packet)) >= 0) {
        if (packet.stream_index == audioStream) {
            mAudioQueue->put(&packet);
        } else if (packet.stream_index == videoStream) {
            mVideoQueue->put(&packet);
        } else {
            // stream that we don't handle, simply free and ignore it
            av_free_packet(&packet);
        }

        // if the queues are full, no need to read more
        if (mAudioQueue->getSize() + mVideoQueue->getSize() > MAX_QUEUE_SIZE
            || ( (mAudioQueue->getNumPackets() > MIN_FRAMES || mAudioStreamIdx < 0 || mAudioQueue->isAbort())
               && (mVideoQueue->getNumPackets() > MIN_FRAMES || mVideoStreamIdx < 0 || mVideoQueue->isAbort() ||
                  (mFormatCtx->streams[mVideoStreamIdx]->disposition & AV_DISPOSITION_ATTACHED_PIC)))) {
            // wait 10 ms
            android::base::System::Duration timeoutMs = 10ll;
            VideoPlayerWaitInfo* pwi = &mContinueReadWaitInfo;
            pwi->lock.lock();
            pwi->done = false;
            const auto deadlineUs = android::base::System::get()->getUnixTimeUs() + timeoutMs * 1000;
            while (!pwi->done && android::base::System::get()->getUnixTimeUs() < deadlineUs) {
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

    /*
    if (mAudioDecoder != nullptr) {
        mAudioDecoder->waitToFinish();
        mAudioDecoder = nullptr;
    }

    if (mVideoDecoder != nullptr) {
        mVideoDecoder->waitToFinish();
        mVideoDecoder = nullptr;
    }
    */

    if (mAudioDecodingThread != nullptr) {
        mAudioDecodingThread->wait();
        mAudioDecodingThread = nullptr;
    }

    if (mVideoDecodingThread != nullptr) {
        mVideoDecodingThread->wait();
        mVideoDecodingThread = nullptr;
    }

    mRunning = false;

    cleanup();

    emit videoFinished();

    return 0;
}

void VideoPlayer::cleanup() {
    if (mAudioFrameQueue != nullptr) {
        mAudioFrameQueue->signalWait();
        mAudioFrameQueue = nullptr;
    }

    if (mVideoFrameQueue != nullptr) {
        mVideoFrameQueue->signalWait();
        mVideoFrameQueue = nullptr;
    }

    mWidget->setPixelBuffer(nullptr, 0);

    if (mImgConvertCtx != nullptr) {
        sws_freeContext(mImgConvertCtx);
        mImgConvertCtx = nullptr;
    }

    if (mSwrCtx != nullptr) {
        swr_free(&mSwrCtx);
        mSwrCtx = nullptr;
    }

    if (mAudioCodecCtx != nullptr) {
        avcodec_close(mAudioCodecCtx);
        mAudioCodecCtx = nullptr;
    }

    if (mVideoCodecCtx != nullptr) {
        avcodec_close(mVideoCodecCtx);
        mVideoCodecCtx = nullptr;
    }

    // close the video file
    if (mFormatCtx != nullptr) {
        avformat_close_input(&mFormatCtx);
        mFormatCtx = nullptr;
    }

    if (mAudioOutputEngine != nullptr) {
        mAudioOutputEngine->close();
        mAudioOutputEngine = nullptr;
    }
}

// get an audio frame from the decoded queue, and convert it to buffer
int VideoPlayer::getConvertedAudioFrame()
{
    Frame *af = mAudioFrameQueue->peekReadable();
    if (af == nullptr) {
        return -1;
    }

    mAudioFrameQueue->next();

    int data_size = av_samples_get_buffer_size(NULL, av_frame_get_channels(af->frame),
                                           af->frame->nb_samples,
                                           (enum AVSampleFormat)af->frame->format, 1);
    int wanted_nb_samples = af->frame->nb_samples;

    int resampled_data_size;

    if (this->mSwrCtx) {
        const uint8_t **in = (const uint8_t **)af->frame->extended_data;
        uint8_t **out = &this->mAudioBuf1;
        int out_count = (int64_t)wanted_nb_samples + 256;
        int out_size  = av_samples_get_buffer_size(NULL, 2, out_count, AV_SAMPLE_FMT_S16, 0);
        if (out_size < 0) {
            return -1;
        }
        av_fast_malloc(&this->mAudioBuf1, &this->mAudioBuf1Size, out_size);
        if (!this->mAudioBuf1) {
            return AVERROR(ENOMEM);
        }
        int len2 = swr_convert(this->mSwrCtx, out, out_count, in, af->frame->nb_samples);
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
    if (!isnan(af->pts)) {
        this->mAudioClockValue = af->pts + (double) af->frame->nb_samples / af->frame->sample_rate;
    } else {
        this->mAudioClockValue = NAN;
    }

    this->mAudioClockSerial = af->serial;

    return resampled_data_size;
}

void VideoPlayer::audioCallback(void* opaque, int len) {
    VideoPlayer* pThis = (VideoPlayer*)opaque;
    pThis->mAudioCallbackTime = av_gettime_relative();

    while (len > 0) {
        if (pThis->mAudioBufIndex >= pThis->mAudioBufSize) {
            int audio_size = pThis->getConvertedAudioFrame();
            if (audio_size < 0) {
                // if error, just output silence, should we output silent buffer
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

        pThis->mAudioOutputEngine->write(pThis->mAudioBuf + pThis->mAudioBufIndex, len1);

        len -= len1;
        pThis->mAudioBufIndex += len1;
    }

    pThis->mAudioWriteBufSize = pThis->mAudioBufSize - pThis->mAudioBufIndex;
}

void VideoPlayer::stop() {
    mRunning = false;

    mTimer.stop();
    internalStop();
}

void VideoPlayer::internalStop() {
    mRunning = false;

    if (mAudioQueue != nullptr) {
        mAudioQueue->signalWait();
    }

    if (mVideoQueue != nullptr) {
        mVideoQueue->signalWait();
    }

    if (mAudioFrameQueue != nullptr) {
        mAudioFrameQueue->signalWait();
    }

    if (mVideoFrameQueue != nullptr) {
        mVideoFrameQueue->signalWait();
    }
}

} // namespace videoplayer
} // namespace android
