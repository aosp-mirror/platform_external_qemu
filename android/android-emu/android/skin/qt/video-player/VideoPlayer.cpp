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

#include "android/skin/qt/video-player/VideoPlayer.h"
#include "android/base/threads/Thread.h"

#include <QDebug>

namespace android {
namespace videoplayer {

//// VideoPlayer implementations

VideoPlayer::VideoPlayer(VideoPlayerWidget* widget, std::string videoFile)
    : mWidget(widget), mVideoFile(videoFile), mRunning(true) {
    mTimer.setSingleShot(true);
    QObject::connect(&mTimer, &QTimer::timeout, this,
                     &VideoPlayer::videoRefreshTimer);
}

VideoPlayer::~VideoPlayer() {}

void VideoPlayer::run() {
    // playVideo("http://commondatastorage.googleapis.com/gtv-videos-bucket/CastVideos/mp4/BigBuckBunny.mp4");
    // playVideo("C:\\Users\\huisi\\Downloads\\worthit.webm");
    // play("/usr/local/google/home/huisinro/Downloads/worthit.webm");
    play(mVideoFile.c_str());
}

// adjust window size to fit the video apect ratio
void VideoPlayer::adjustWindowSize(AVCodecContext* pVideoCodecCtx,
                                   int* pWidth,
                                   int* pHeight) {
    float aspect_ratio;

    if (pVideoCodecCtx->sample_aspect_ratio.num == 0) {
        aspect_ratio = 0;
    } else {
        aspect_ratio = av_q2d(pVideoCodecCtx->sample_aspect_ratio) *
                       pVideoCodecCtx->width / pVideoCodecCtx->height;
    }

    if (aspect_ratio <= 0.0) {
        aspect_ratio =
                (float)pVideoCodecCtx->width / (float)pVideoCodecCtx->height;
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

// decode audio frame
void VideoPlayer::decodeAudioThread() {
    AVPacket packet;
    while (mRunning) {
        if (mAudioQueue->get(&packet, true, nullptr) < 0) {
            break;
        }

        if (packet.data == PacketQueue::sFlushPkt.data) {
            continue;
        }

        // last eof packaet
        if (packet.data == nullptr || packet.size == 0) {
            break;
        }

        int got_frame;
        int len = avcodec_decode_audio4(pAudioCodecCtx, pAudioFrame, &got_frame,
                                        &packet);
        if (len >= 0 && got_frame) {
            AVRational tb = (AVRational){1, pAudioFrame->sample_rate};
            if (pAudioFrame->pts != AV_NOPTS_VALUE)
                pAudioFrame->pts = av_rescale_q(pAudioFrame->pts, pAudioCodecCtx->time_base, tb);
            else if (pAudioFrame->pkt_pts != AV_NOPTS_VALUE)
                pAudioFrame->pts = av_rescale_q(pAudioFrame->pkt_pts, av_codec_get_pkt_timebase(pAudioCodecCtx), tb);
            else if (this->next_pts != AV_NOPTS_VALUE)
                pAudioFrame->pts = av_rescale_q(this->next_pts, this->next_pts_tb, tb);
            if (pAudioFrame->pts != AV_NOPTS_VALUE) {
                this->next_pts = pAudioFrame->pts + pAudioFrame->nb_samples;
                this->next_pts_tb = tb;
            }

            Frame *af = mAudioFrameQueue->peekWritable();
            if (af == nullptr) {
                break;
            }

            af->pts = (pAudioFrame->pts == AV_NOPTS_VALUE) ? NAN : pAudioFrame->pts * av_q2d(tb);
            af->pos = av_frame_get_pkt_pos(pAudioFrame);
            //af->serial = is->auddec.pkt_serial;
            af->duration = av_q2d((AVRational){pAudioFrame->nb_samples, pAudioFrame->sample_rate});

            av_frame_move_ref(af->frame, pAudioFrame);
            mAudioFrameQueue->push();

            audio_frame_count++;
        }

        av_free_packet(&packet);
    }
}

// decode video frame
void VideoPlayer::decodeVideoThread() {
    AVPacket packet;
    while (mRunning) {
        if (mVideoQueue->get(&packet, true, nullptr) < 0) {
            break;
        }

        if (packet.data == PacketQueue::sFlushPkt.data) {
            continue;
        }

        // last eof packaet
        if (packet.data == nullptr || packet.size == 0) {
            break;
        }

        int got_frame;
        int len = avcodec_decode_video2(pVideoCodecCtx, pVideoFrame, &got_frame,
                                        &packet);
        if (len >= 0 && got_frame) {
            double pts = pVideoFrame->pkt_pts;
            if (pts == AV_NOPTS_VALUE) {
                if ((pts = av_frame_get_best_effort_timestamp(pVideoFrame)) ==
                    AV_NOPTS_VALUE) {
                    pts = 0;
                }
            }
            pts *= av_q2d(pFormatCtx->streams[mVideoStreamIdx]->time_base);
            pts = synchronizeVideo(pVideoFrame, pts);
            if (queuePicture(pVideoFrame, pts) < 0) {
                av_free_packet(&packet);
                break;
            }
        }

        av_free_packet(&packet);
    }
}

int VideoPlayer::queuePicture(AVFrame* pFrame, double pts) {
    mNumVideoFrames++;

    // wait until we have space for a new pic
    struct VideoPlayerWaitInfo* pwi = &mFrameQueueWaitInfo;

    pwi->lock.lock();
    // pwi->done = false;
    while (pictq_size >= VIDEO_PICTURE_QUEUE_SIZE && mRunning) {
        pwi->cvDone.wait(&pwi->lock);
    }
    pwi->lock.unlock();

    if (!mRunning) {
        return -1;
    }

    // windex is set to 0 initially
    VideoPicture* vp = &pictq[pictq_windex];

    // allocate or resize the buffer
    /*
    if (vp->buf == nullptr ||
        vp->width != pVideoCodecCtx->width ||
        vp->height != pVideoCodecCtx->height) {
        vp->width = pVideoCodecCtx->width;
        vp->height = pVideoCodecCtx->height;
        if (!mRunning) {
            return -1;
        }
    }
    */

    if (vp->buf) {
        vp->pts = pts;

        // Assign appropriate parts of buffer to image planes in pFrameRGB
        AVPicture pict;
        avpicture_fill(&pict, vp->buf + vp->headerlen, AV_PIX_FMT_RGB24,
                       vp->width, vp->height);

        // Convert the image to RGB format
        sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0,
                  pVideoCodecCtx->height, pict.data, pict.linesize);

        // displayVideoFrame(vp);

        // we inform our display thread that we have a pic ready
        if (++this->pictq_windex == VIDEO_PICTURE_QUEUE_SIZE) {
            this->pictq_windex = 0;
        }

        pwi->lock.lock();
        this->pictq_size++;
        pwi->lock.unlock();
    }

    return 0;
}

double VideoPlayer::synchronizeVideo(AVFrame* frame, double pts) {
    double frame_delay;

    if (pts != 0) {
        // if we have pts, set video clock to it
        video_clock = pts;
    } else {
        // if we aren't given a pts, set it to the clock
        pts = video_clock;
    }

    // update the video clock
    frame_delay = av_q2d(pVideoCodecCtx->time_base);

    // if we are repeating a frame, adjust clock accordingly
    frame_delay += frame->repeat_pict * (frame_delay * 0.5);
    video_clock += frame_delay;

    return pts;
}

void VideoPlayer::displayVideoFrame(VideoPicture* vp) {
    // static double last_time_ms = 0;
    // double elapsed = (av_gettime() / 1000.0) - last_time_ms;
    // render the video
    mWidget->setPixelBuffer(vp->buf, vp->len);

    emit updateWidget();

    if (current_audio_wait == 0) {
        this->msleep(20);
    }
    current_audio_wait = 0;

    mNumVideoPicts++;
    // last_time_ms = (av_gettime() / 1000.0);
}

void VideoPlayer::videoRefreshTimer() {
    if (!mRunning && this->pictq_size == 0) {
        mTimer.stop();
        return;
    }

    VideoPicture* vp;
    double actual_delay, delay, sync_threshold;

    if (this->mVideoStreamIdx == -1) {
        scheduleRefresh(100);
        return;
    }

    if (this->pictq_size == 0) {
        scheduleRefresh(50);
    } else {
        vp = &this->pictq[this->pictq_rindex];
        // the pts from last time
        delay = vp->pts - this->frame_last_pts;
        double delay2 = delay;
        if (delay <= 0 || delay >= 1.0) {
            // if incorrect delay, use previous one
            delay = this->frame_last_delay;
        }
        // save for next time
        this->frame_last_delay = delay;
        this->frame_last_pts = vp->pts;

        double diff = 0.0;

        // update delay to sync to audio
        if (mAudioStreamIdx != -1) {
            // diff = vp->pts - getAudioClock(is);
        }

        // skip or repeat the frame. Take delay into account
        // still doesn't "know if this is the best guess."
        sync_threshold =
                (delay > AV_SYNC_THRESHOLD) ? delay : AV_SYNC_THRESHOLD;
        if (fabs(diff) < AV_NOSYNC_THRESHOLD) {
            if (diff <= -sync_threshold) {
                delay = 0;
            } else if (diff >= sync_threshold) {
                delay = 2 * delay;
            }
        }
        this->frame_timer += delay;
        /* computer the REAL delay */
        actual_delay = this->frame_timer - (av_gettime() / 1000000.0);
        if (actual_delay < 0.010) {
            /* Really it should skip the picture instead */
            actual_delay = 0.010;
        }

        // scheduleRefresh((int)(actual_delay * 1000 + 0.5));
        scheduleRefresh(delay2 * 1000/*40*/);

        /* show the picture! */
        displayVideoFrame(vp);

        /* update queue for next picture! */
        if (++this->pictq_rindex == VIDEO_PICTURE_QUEUE_SIZE) {
            this->pictq_rindex = 0;
        }

        struct VideoPlayerWaitInfo* pwi = &mFrameQueueWaitInfo;
        pwi->lock.lock();
        this->pictq_size--;
        // pwi->done = true;
        pwi->cvDone.signalAndUnlock(&pwi->lock);
    }
}

// schedule a timer to start in the specified milliseconds
void VideoPlayer::scheduleRefresh(int delayMs) {
    if (mRunning) {
        mTimer.start(delayMs);
    } else {
        mTimer.stop();
    }
}

int VideoPlayer::play(const char* filename) {
    // Register all formats and codecs
    av_register_all();
    avformat_network_init();

    PacketQueue::init();

    if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0) {
        return -1;  // failed to open video file
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        return -1;  // failed to find stream info
    }

    // Dump video format
    av_dump_format(pFormatCtx, 0, filename, false);

    mRunning = true;

    // Find the first audio/video stream
    int audioStream = -1;
    int videoStream = -1;
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStream = i;
            if (videoStream != -1) {
                break;
            }
        } else if (pFormatCtx->streams[i]->codec->codec_type ==
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

    int dst_linesize;

    mAudioOutputEngine = AudioOutputEngine::get();

    if (audioStream != -1) {
        mAudioStreamIdx = audioStream;
        pAudioCodecCtx = pFormatCtx->streams[audioStream]->codec;

        // Find the decoder for the video stream
        pAudioCodec = avcodec_find_decoder(pAudioCodecCtx->codec_id);
        if (pAudioCodec == NULL) {
            return -1;
        }

        // Open codec
        if (avcodec_open2(pAudioCodecCtx, pAudioCodec, NULL) < 0) {
            return -1;
        }

        // Allocate audio frame
        pAudioFrame = av_frame_alloc();

        if (mAudioOutputEngine != nullptr) {
            mAudioOutputEngine->open("video-player",
                android::emulation::AudioFormat::S16, pAudioCodecCtx->sample_rate,
                pAudioCodecCtx->channels, audioCallback, this);
        }

        av_samples_alloc(&resampleBuffer, &dst_linesize,
                         pAudioCodecCtx->channels, dst_nb_samples,
                         AV_SAMPLE_FMT_S16, 0);

        // create resampler context
        swr_ctx = swr_alloc();
        if (swr_ctx == NULL) {
            qDebug() << "Could not allocate resampler context";
            return -1;
        }

        // set options
        av_opt_set_int(swr_ctx, "in_channel_count", pAudioCodecCtx->channels,
                       0);
        av_opt_set_int(swr_ctx, "in_sample_rate", pAudioCodecCtx->sample_rate,
                       0);
        av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt",
                              pAudioCodecCtx->sample_fmt, 0);
        av_opt_set_int(swr_ctx, "out_channel_count", pAudioCodecCtx->channels,
                       0);
        av_opt_set_int(swr_ctx, "out_sample_rate", pAudioCodecCtx->sample_rate,
                       0);
        av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

        // initialize the resampling context
        if (swr_init(swr_ctx) < 0) {
            qDebug() << "Failed to initialize the resampling context";
            return -1;
        }

        mAudioQueue.reset(new PacketQueue(this));
        mAudioQueue->start();

        mAudioFrameQueue.reset(new FrameQueue(this, mAudioQueue.get(), AUDIO_SAMPLE_QUEUE_SIZE, true));

        mAudioDecoder.reset(new Decoder(this, pAudioCodecCtx, mAudioQueue.get(), nullptr));
        //mAudioDecoder->startThread(ThreadType::AudioDecoding);
        mAudioDecodingThread.reset(new ThreadWrapper(this, ThreadType::AudioDecoding));
        mAudioDecodingThread->start();
    }

    if (videoStream != -1) {
        mVideoStreamIdx = videoStream;

        pVideoCodecCtx = pFormatCtx->streams[videoStream]->codec;

        // Find the decoder for the video stream
        pVideoCodec = avcodec_find_decoder(pVideoCodecCtx->codec_id);
        if (pVideoCodec == NULL) {
            return -1;
        }

        // Open codec
        if (avcodec_open2(pVideoCodecCtx, pVideoCodec, NULL) < 0) {
            return -1;
        }

        // Allocate video frame
        pVideoFrame = av_frame_alloc();

        // Allocate an AVFrame structure
        pFrameRGB = av_frame_alloc();
        if (pFrameRGB == NULL) {
            return -1;
        }

        AVPixelFormat dst_fmt = AV_PIX_FMT_RGB24;
        int dst_w = pVideoCodecCtx->width;
        int dst_h = pVideoCodecCtx->height;
        adjustWindowSize(pVideoCodecCtx, &dst_w, &dst_h);

        // Determine required buffer size and allocate buffer
        numBytes = avpicture_get_size(dst_fmt, dst_w, dst_h);
        buffer = new unsigned char[numBytes + 64];
        int headerlen = 0;
        if (buffer != nullptr) {
            // append a PPM header to it's a ppm image format
            headerlen =
                    sprintf((char*)buffer, "P6\n%d %d\n255\n", dst_w, dst_h);
        }

        mWidget->setPixelBuffer(buffer, numBytes + headerlen);

        // assign appropriate parts of buffer to image planes in pFrameRGB
        avpicture_fill((AVPicture*)pFrameRGB, buffer + headerlen, dst_fmt,
                       dst_w, dst_h);
        this->pictq_size = 0;
        this->pictq_rindex = 0;
        this->pictq_windex = 0;

        for (int i = 0; i < VIDEO_PICTURE_QUEUE_SIZE; i++) {
            this->pictq[i].buf = new unsigned char[numBytes + 64];
            if (this->pictq[i].buf != nullptr) {
                // simply append a PPM header to become ppm image format
                headerlen = sprintf((char*)this->pictq[i].buf,
                                    "P6\n%d %d\n255\n", dst_w, dst_h);
            } else {
                headerlen = 0;
            }
            this->pictq[i].headerlen = headerlen;
            this->pictq[i].len = numBytes + headerlen;
            this->pictq[i].width = dst_w;
            this->pictq[i].height = dst_h;
        }

        img_convert_ctx =
                sws_getContext(pVideoCodecCtx->width, pVideoCodecCtx->height,
                               pVideoCodecCtx->pix_fmt, dst_w, dst_h, dst_fmt,
                               SWS_FAST_BILINEAR, NULL, NULL, NULL);

        this->frame_timer = (double)av_gettime() / 1000000.0;
        this->frame_last_delay = 40e-3;

        mVideoQueue.reset(new PacketQueue(this));
        mVideoQueue->start();

        mVideoFrameQueue.reset(new FrameQueue(this, mVideoQueue.get(), VIDEO_PICTURE_QUEUE_SIZE, true));

        mVideoDecoder.reset(new Decoder(this, pVideoCodecCtx, mVideoQueue.get(), nullptr));
        //mVideoDecoder->startThread(ThreadType::VideoDecoding); // weird, this does not work well
        mVideoDecodingThread.reset(new ThreadWrapper(this, ThreadType::VideoDecoding));
        mVideoDecodingThread->start();
    }

    // Read frames from the video file
    int ret = 0;
    AVPacket packet;
    while (mRunning && (ret = av_read_frame(pFormatCtx, &packet)) >= 0) {
        if (packet.stream_index == audioStream) {
            mAudioQueue->put(&packet);
        } else if (packet.stream_index == videoStream) {
            mVideoQueue->put(&packet);
        } else {
            // stream that we don't handle, simply free and ignore it
            av_free_packet(&packet);
        }
    }

    if (ret == AVERROR_EOF || avio_feof(pFormatCtx->pb)) {
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
    // Free the RGB image
    if (buffer != nullptr) {
        delete[] buffer;
        buffer = nullptr;
    }

    if (mAudioFrameQueue != nullptr) {
        mAudioFrameQueue->signalWait();
        mAudioFrameQueue = nullptr;
    }

    if (mVideoFrameQueue != nullptr) {
        mVideoFrameQueue->signalWait();
        mVideoFrameQueue = nullptr;
    }

    mWidget->setPixelBuffer(nullptr, 0);

    if (pFrameRGB != nullptr) {
        av_free(pFrameRGB);
        pFrameRGB = nullptr;
    }

    if (pVideoFrame != nullptr) {
        av_free(pVideoFrame);
        pVideoFrame = nullptr;
    }

    if (img_convert_ctx != nullptr) {
        sws_freeContext(img_convert_ctx);
        img_convert_ctx = nullptr;
    }

    if (swr_ctx != nullptr) {
        swr_free(&swr_ctx);
        swr_ctx = nullptr;
    }

    if (pAudioCodecCtx != nullptr) {
        avcodec_close(pAudioCodecCtx);
        pAudioCodecCtx = nullptr;
    }

    if (pVideoCodecCtx != nullptr) {
        avcodec_close(pVideoCodecCtx);
        pVideoCodecCtx = nullptr;
    }

    // close the video file
    if (pFormatCtx != nullptr) {
        avformat_close_input(&pFormatCtx);
        pFormatCtx = nullptr;
    }

    for (int i = 0; i < VIDEO_PICTURE_QUEUE_SIZE; i++) {
        delete[] this->pictq[i].buf;
    }

    if (mAudioOutputEngine != nullptr) {
        mAudioOutputEngine->close();
        mAudioOutputEngine = nullptr;
    }
}

// get an audio frame from the decoded queue, and convert it to buffer
int VideoPlayer::getConvertedAudioFrame()
{
    int resampled_data_size;
    av_unused double audio_clock0;

    Frame *af = mAudioFrameQueue->peekReadable();
    if (af == nullptr) {
        return -1;
    }

    mAudioFrameQueue->next();

    int data_size = av_samples_get_buffer_size(NULL, av_frame_get_channels(af->frame),
                                           af->frame->nb_samples,
                                           (enum AVSampleFormat)af->frame->format, 1);
    int wanted_nb_samples = af->frame->nb_samples;

    if (this->swr_ctx) {
        const uint8_t **in = (const uint8_t **)af->frame->extended_data;
        uint8_t **out = &this->audio_buf1;
        int out_count = (int64_t)wanted_nb_samples + 256;
        int out_size  = av_samples_get_buffer_size(NULL, 2, out_count, AV_SAMPLE_FMT_S16, 0);
        if (out_size < 0) {
            return -1;
        }
        av_fast_malloc(&this->audio_buf1, &this->audio_buf1_size, out_size);
        if (!this->audio_buf1)
            return AVERROR(ENOMEM);
        int len2 = swr_convert(this->swr_ctx, out, out_count, in, af->frame->nb_samples);
        if (len2 < 0) {
            return -1;
        }
        if (len2 == out_count) {
            if (swr_init(this->swr_ctx) < 0)
                swr_free(&this->swr_ctx);
        }
        this->audio_buf = this->audio_buf1;
        resampled_data_size = len2 * 2 * 2;
    } else {
        this->audio_buf = af->frame->data[0];
        resampled_data_size = data_size;
    }

    audio_clock0 = this->audio_clock;
    // update the audio clock with the pts
    if (!isnan(af->pts))
        this->audio_clock = af->pts + (double) af->frame->nb_samples / af->frame->sample_rate;
    else
        this->audio_clock = NAN;

    this->audio_clock_serial = af->serial;

    return resampled_data_size;
}

void VideoPlayer::audioCallback(void* opaque, int len) {
    VideoPlayer* pThis = (VideoPlayer*)opaque;
    pThis->mAudioCallbackTime = av_gettime_relative();

    while (len > 0) {
        if (pThis->audio_buf_index >= pThis->audio_buf_size) {
            int audio_size = pThis->getConvertedAudioFrame();
            if (audio_size < 0) {
                // if error, just output silence, should we output silent buffer
                break;
            } else {
                pThis->audio_buf_size = audio_size;
            }
            pThis->audio_buf_index = 0;
        }

        int len1 = pThis->audio_buf_size - pThis->audio_buf_index;
        if (len1 > len) {
            len1 = len;
        }

        pThis->mAudioOutputEngine->write(pThis->audio_buf + pThis->audio_buf_index, len1);

        len -= len1;
        pThis->audio_buf_index += len1;
    }

    pThis->audio_write_buf_size = pThis->audio_buf_size - pThis->audio_buf_index;
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

    struct VideoPlayerWaitInfo* pwi = &mFrameQueueWaitInfo;
    pwi->lock.lock();
    pwi->cvDone.signalAndUnlock(&pwi->lock);
}

} // namespace videoplayer
} // namespace android
