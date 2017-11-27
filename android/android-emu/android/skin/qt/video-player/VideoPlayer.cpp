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

#include "android/skin/qt/video-player/VideoPlayer.h"
#include "android/base/threads/Thread.h"

#include <QDebug>

namespace android {
namespace videoplayer {

//// PacketQueue implementations

PacketQueue::PacketQueue(VideoPlayer* player) : mPlayer(player) {}

PacketQueue::~PacketQueue() {}

// insert a new packet
int PacketQueue::put(AVPacket* pkt) {
    AVPacketList* pkt1 = (AVPacketList*)av_malloc(sizeof(AVPacketList));
    if (pkt1 == nullptr)
        return -1;

    pkt1->pkt = *pkt;
    pkt1->next = NULL;

    struct VideoPlayerWaitInfo* pwi = &mWaitInfo;
    pwi->lock.lock();

    if (mLastPkt == nullptr)
        mFirstPkt = pkt1;
    else
        mLastPkt->next = pkt1;

    mLastPkt = pkt1;
    mNumPkts++;
    mSize += pkt1->pkt.size;

    pwi->done = true;
    pwi->cvDone.signalAndUnlock(&pwi->lock);

    return 0;
}

// retrieve the front packet, usually for decoding
int PacketQueue::get(AVPacket* pkt, bool blocking) {
    int ret;
    struct VideoPlayerWaitInfo* pwi = &mWaitInfo;
    android::base::AutoLock lock(pwi->lock);

    while (true) {
        if (!mPlayer->isRunning()) {
            ret = -1;
            break;
        }

        AVPacketList* pkt1 = mFirstPkt;
        if (pkt1) {
            mFirstPkt = pkt1->next;
            if (mFirstPkt == nullptr)
                mLastPkt = NULL;
            mNumPkts--;
            mSize -= pkt1->pkt.size;
            *pkt = pkt1->pkt;
            av_free(pkt1);
            ret = 1;
            break;
        } else if (!blocking) {
            ret = 0;
            break;
        } else {
            pwi->done = false;
            while (!pwi->done) {
                pwi->cvDone.wait(&pwi->lock);
            }
        }
    }

    return ret;
}

// signal and unlock blocking wait
void PacketQueue::signalWait() {
    struct VideoPlayerWaitInfo* pwi = &mWaitInfo;
    pwi->lock.lock();
    pwi->done = true;
    pwi->cvDone.signalAndUnlock(&pwi->lock);
}

//// FrameQueue implementations

FrameQueue::FrameQueue(VideoPlayer* player,
                       PacketQueue* pktq,
                       int max_size,
                       bool keep_last)
    : mPlayer(player), mPktq(pktq), mKeepLast(keep_last) {
    mMaxSize = FFMIN(max_size, FRAME_QUEUE_SIZE);
    for (int i = 0; i < mMaxSize; i++) {
        mQueue[i].frame = av_frame_alloc();
    }
}

FrameQueue::~FrameQueue() {
    for (int i = 0; i < mMaxSize; i++) {
        Frame* vp = &mQueue[i];
        av_frame_unref(vp->frame);
        av_frame_free(&vp->frame);
    }
}

void FrameQueue::signal() {
    struct VideoPlayerWaitInfo* pwi = &mWaitInfo;
    pwi->lock.lock();
    pwi->done = true;
    pwi->cvDone.signalAndUnlock(&pwi->lock);
}

Frame* FrameQueue::peek() {
    return &mQueue[(mRindex + mRindexShown) % mMaxSize];
}

Frame* FrameQueue::peekNext() {
    return &mQueue[(mRindex + mRindexShown + 1) % mMaxSize];
}

Frame* FrameQueue::peekLast() {
    return &mQueue[mRindex];
}

Frame* FrameQueue::peekWritable() {
    // wait until we have space to put a new frame
    struct VideoPlayerWaitInfo* pwi = &mWaitInfo;
    pwi->lock.lock();
    while (mSize >= mMaxSize &&
           !mPlayer->mRunning /*&& !pktq->abort_request*/) {
        pwi->cvDone.wait(&pwi->lock);
    }
    pwi->lock.unlock();

    if (!mPlayer->mRunning /* || pktq->abort_request*/)
        return nullptr;

    return &mQueue[mWindex];
}

Frame* FrameQueue::peekReadable() {
    // wait until we have a readable a new frame
    struct VideoPlayerWaitInfo* pwi = &mWaitInfo;
    pwi->lock.lock();
    while (mSize - mRindexShown <= 0 &&
           !mPlayer->mRunning /*&& !pktq->abort_request*/) {
        pwi->cvDone.wait(&pwi->lock);
    }
    pwi->lock.unlock();

    if (!mPlayer->mRunning /* || pktq->abort_request*/)
        return nullptr;

    return &mQueue[(mRindex + mRindexShown) % mMaxSize];
}

void FrameQueue::push() {
    if (++mWindex == mMaxSize)
        mWindex = 0;

    struct VideoPlayerWaitInfo* pwi = &mWaitInfo;
    pwi->lock.lock();
    mSize++;
    pwi->cvDone.signalAndUnlock(&pwi->lock);
}

void FrameQueue::next() {
    if (mKeepLast && !mRindexShown) {
        mRindexShown = 1;
        return;
    }
    av_frame_unref(mQueue[mRindex].frame);
    if (++mRindex == mMaxSize)
        mRindex = 0;

    struct VideoPlayerWaitInfo* pwi = &mWaitInfo;
    pwi->lock.lock();
    mSize--;
    pwi->cvDone.signalAndUnlock(&pwi->lock);
}

// jump back to the previous frame if available by resetting rindex_shown
int FrameQueue::prev() {
    int ret = mRindexShown;
    mRindexShown = 0;
    return ret;
}

// return the number of unrendered frames in the queue
int FrameQueue::numRemaining() {
    return mSize - mRindexShown;
}

// return last shown position
int64_t FrameQueue::lastPos() {
    Frame* fp = &mQueue[mRindex];
    if (mRindexShown && fp->serial == mPktq->mSerial)
        return fp->pos;
    else
        return -1;
}

// ThreadWrapper implementations
intptr_t ThreadWrapper::main() {
    if (mKind == 0) {
        mPlayer->decodeAudioThread();
    } else if (mKind == 1) {
        mPlayer->decodeVideoThread();
    }

    return 0;
}

//// VideoPlayer implementations

VideoPlayer::VideoPlayer(VideoPlayerWidget* widget, std::string videoFile)
    : mWidget(widget), mVideoFile(videoFile), mRunning(true) {
    mTimer.setSingleShot(true);
    QObject::connect(&mTimer, &QTimer::timeout, this,
                     &VideoPlayer::videoRefreshTimer);
}

VideoPlayer::~VideoPlayer() {}

void VideoPlayer::run() {
    // playVideo("http://commondatastorage.googleapis.com/gtv-videos-bucket/CastVideos/mp4/BigBuckBunny.mp4"/*"test.webm"*/);
    // playVideo("C:\\Users\\huisi\\Downloads\\worthit.webm");
    // play("/usr/local/google/home/huisinro/Downloads/worthit.webm");
    int rc = play(mVideoFile.c_str());
    printf("play(), rc=%d\n", rc);
}

static QByteArray* g_audio_buffer = nullptr;

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
        if (mAudioQueue->get(&packet, true) < 0) {
            break;
        }

        // last eof packaet
        if (packet.data == nullptr || packet.size == 0) {
            break;
        }

        int got_frame;
        int len = avcodec_decode_audio4(pAudioCodecCtx, pAudioFrame, &got_frame,
                                        &packet);
        if (len >= 0 && got_frame) {
            int samples_output =
                    swr_convert(swr_ctx, &resampleBuffer, dst_nb_samples,
                                (const uint8_t**)pAudioFrame->data,
                                pAudioFrame->nb_samples);

            // if (audio_frame_count != 0) {
            //    struct VideoPlayerWaitInfo* pwi = &mAudioBufferDoneWaitInfo;
            //    android::base::AutoLock lock(pwi->lock);
            //    pwi->done = false;
            //    while (!pwi->done) {
            //        pwi->cvDone.wait(&pwi->lock);
            //    }
            //}

            audio_frame_count++;

            if (g_audio_buffer != nullptr) {
                delete g_audio_buffer;
            }

            g_audio_buffer = new QByteArray((const char*)resampleBuffer,
                                            samples_output * 4);

            // struct VideoPlayerWaitInfo* pwi = &mAudioAvailableWaitInfo;
            // android::base::AutoLock lock(pwi->lock);
            // pwi->done = true;
            // pwi->cvDone.signalAndUnlock(&pwi->lock);

            current_audio_wait =
                    (samples_output * 1000) / pAudioCodecCtx->sample_rate;
            this->msleep(current_audio_wait);
        }

        av_free_packet(&packet);
    }

    printf("decodeAudioThread(): Finished decoding all audio frames.\n");
}

// decode video frame
void VideoPlayer::decodeVideoThread() {
    AVPacket packet;
    while (mRunning) {
//printf("VideoPlayer::decodeVideoThread() 1\n");
        if (mVideoQueue->get(&packet, true) < 0) {
//printf("VideoPlayer::decodeVideoThread() 2\n");
            break;
        }
//printf("VideoPlayer::decodeVideoThread() 3\n");

        // last eof packaet
        if (packet.data == nullptr || packet.size == 0) {
            break;
        }        

        int got_frame;
//printf("VideoPlayer::decodeVideoThread(), avcodec_decode_video2 enter\n");
        int len = avcodec_decode_video2(pVideoCodecCtx, pVideoFrame, &got_frame,
                                        &packet);
//printf("VideoPlayer::decodeVideoThread(), avcodec_decode_video2 exit\n");
        if (len >= 0 && got_frame) {
            double pts = 0;
            if ((pts = av_frame_get_best_effort_timestamp(pVideoFrame)) ==
                AV_NOPTS_VALUE) {
                pts = 0;
            }
            pts *= av_q2d(pFormatCtx->streams[mVideoStreamIdx]->time_base);
            pts = synchronizeVideo(pVideoFrame, pts);

            // Convert the image to RGB format
            // sws_scale(img_convert_ctx, pVideoFrame->data,
            // pVideoFrame->linesize, 0,
            //          pVideoCodecCtx->height, pFrameRGB->data,
            //          pFrameRGB->linesize);
            // emit updateWidget();
            // sleep(20);
//printf("VideoPlayer::decodeVideoThread(), queuePicture enter\n");
            if (queuePicture(pVideoFrame, pts) < 0) {
//printf("VideoPlayer::decodeVideoThread(), queuePicture exit\n");
                av_free_packet(&packet);
                break;
            }
//printf("VideoPlayer::decodeVideoThread(), queuePicture exit\n");
        }

        av_free_packet(&packet);
    }

    printf("decodeVideoThread(): Finished decoding all video frames.\n");
}

int VideoPlayer::queuePicture(AVFrame* pFrame, double pts) {
    // printf("queuePicture() enter: mNumVideoFrames=%d\n", mNumVideoFrames);

    mNumVideoFrames++;

    // wait until we have space for a new pic
    struct VideoPlayerWaitInfo* pwi = &mFrameQueueWaitInfo;

    pwi->lock.lock();
    // pwi->done = false;
    while (pictq_size >= VIDEO_PICTURE_QUEUE_SIZE && mRunning) {
        // printf("queuePicture(): picq is full, waiting...\n");
        pwi->cvDone.wait(&pwi->lock);
        // printf("queuePicture(): picq waiting finished.\n");
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

        // printf("queuePicture(): vp->width=%d, vp->height=%d,
        // vp->headerlen=%d\n", vp->width, vp->height, vp->headerlen);

        // Convert the image to RGB format
        sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0,
                  pVideoCodecCtx->height, pict.data, pict.linesize);

        // displayVideoFrame(vp);

        // we inform our display thread that we have a pic ready
        if (++this->pictq_windex == VIDEO_PICTURE_QUEUE_SIZE) {
            this->pictq_windex = 0;
        }

        // printf("queuePicture(): lock\n");
        pwi->lock.lock();
        this->pictq_size++;
        pwi->lock.unlock();
        // printf("queuePicture(): unlock\n");
    }

    // printf("queuePicture() exit: mNumVideoFrames=%d\n", mNumVideoFrames);

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

    // printf("displayVideoFrame(): mNumVideoPicts=%d, elapsed=%fms, vp->buf=%p,
    // vp->len=%d\n", mNumVideoPicts, elapsed, vp->buf, vp->len);

    // render the video
    mWidget->buf = vp->buf;
    mWidget->len = vp->len;
    // memcpy(mWidget->buf, vp->buf, vp->len);

    emit updateWidget();

    if (current_audio_wait == 0) {
        this->msleep(20);
    }
    current_audio_wait = 0;

    mNumVideoPicts++;

    // last_time_ms = (av_gettime() / 1000.0);
}

void VideoPlayer::videoRefreshTimer() {
    // printf("videoRefreshTimer(): pictq_size=%d, pictq_rindex=%d\n",
    // this->pictq_size, pictq_rindex);

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

        delay = vp->pts - this->frame_last_pts; /* the pts from last time */
        if (delay <= 0 || delay >= 1.0) {
            /* if incorrect delay, use previous one */
            delay = this->frame_last_delay;
        }
        /* save for next time */
        this->frame_last_delay = delay;
        this->frame_last_pts = vp->pts;

        double diff = 0.0;

        /* update delay to sync to audio */
        if (mAudioStreamIdx != -1) {
            // diff = vp->pts - getAudioClock(is);
        }

        /* Skip or repeat the frame. Take delay into account
           still doesn't "know if this is the best guess." */
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
        scheduleRefresh(40);

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
        // printf("scheduleRefresh(delayMs=%d)\n", delayMs);
        mTimer.start(delayMs);
    } else {
        mTimer.stop();
    }
}

int VideoPlayer::play(const char* filename) {
    printf("VideoPlayer::play(filename=%s)\n", filename);

    // Register all formats and codecs
    av_register_all();
    avformat_network_init();

    if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0)
        return -1;  // failed to open video file

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
        return -1;  // failed to find stream info

    // Dump video format
    av_dump_format(pFormatCtx, 0, filename, false);

    mRunning = true;

    // Find the first audio/video stream
    int audioStream = -1;
    int videoStream = -1;
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStream = i;
            if (videoStream != -1)
                break;
        } else if (pFormatCtx->streams[i]->codec->codec_type ==
                   AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            if (audioStream != -1)
                break;
        }
    }

    if (videoStream == -1 && audioStream == -1) {
        return -1;  // no audio or video stream found
    }

    int dst_linesize;

    // mAudioOutputEngine = AudioOutputEngine::get();

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

        // if (mAudioOutputEngine != nullptr) {
        //    mAudioOutputEngine->open("video-player",
        //    android::emulation::AudioFormat::S16, pAudioCodecCtx->sample_rate,
        //    pAudioCodecCtx->channels, audioCallback, this);
        //}

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

        mAudioDecodingThread.reset(new ThreadWrapper(this, 0));
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

        mWidget->buf = (unsigned char*)buffer;
        mWidget->len = numBytes + headerlen;

        // assign appropriate parts of buffer to image planes in pFrameRGB
        avpicture_fill((AVPicture*)pFrameRGB, buffer + headerlen, dst_fmt,
                       dst_w, dst_h);

        printf("VideoPlayer::play(): dst_w=%d, dst_h=%d, headerlen=%d\n", dst_w,
               dst_h, headerlen);

        this->pictq_size = 0;
        this->pictq_rindex = 0;
        this->pictq_windex = 0;

        for (int i = 0; i < VIDEO_PICTURE_QUEUE_SIZE; i++) {
            this->pictq[i].buf = new unsigned char[numBytes + 64];
            if (this->pictq[i].buf != nullptr) {
                // append a PPM header to become ppm image format
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

        mVideoDecodingThread.reset(new ThreadWrapper(this, 1));
        mVideoDecodingThread->start();
    }

    // Read frames and save first five frames to disk
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
        packet.data = nullptr;
        packet.size = 0;
        packet.stream_index = videoStream;
        mVideoQueue->put(&packet);
        printf("Insert a null frame to video queue\n");

        packet.data = nullptr;
        packet.size = 0;
        packet.stream_index = audioStream;
        mAudioQueue->put(&packet);
        printf("Insert a null frame to audio queue\n");
    }

    printf("Finished reading all frames.\n");

    if (mAudioDecodingThread != nullptr) {
        mAudioDecodingThread->wait();
        mAudioDecodingThread = nullptr;
    }

    if (mVideoDecodingThread != nullptr) {
        mVideoDecodingThread->wait();
        mVideoDecodingThread = nullptr;
    }

    cleanup();

    emit videoFinished();

    return 0;
}

void VideoPlayer::cleanup() {
    printf("VideoPlayer::cleanup()\n");

    // mAudioOutputEngine->close();
    
    // Free the RGB image
    if (buffer != nullptr) {
        delete[] buffer;
        buffer = nullptr;
    }

    mWidget->buf = nullptr;
    mWidget->len = 0;

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

    // Close the video file
    if (pFormatCtx != nullptr) {
        avformat_close_input(&pFormatCtx);
        pFormatCtx = nullptr;
    }

    for (int i = 0; i < VIDEO_PICTURE_QUEUE_SIZE; i++) {
        delete[] this->pictq[i].buf;
    }
}

void VideoPlayer::audioCallback(void* opaque, int free) {
    VideoPlayer* pThis = (VideoPlayer*)opaque;
    if (g_audio_buffer == nullptr) {
        return;
    }

    struct VideoPlayerWaitInfo* pwi = &pThis->mAudioAvailableWaitInfo;
    android::base::AutoLock lock(pwi->lock);
    pwi->done = false;
    while (!pwi->done) {
        pwi->cvDone.wait(&pwi->lock);
    }

    int size = g_audio_buffer->size();
    char* data = /*g_audio_buffer->data(); */ new char[size];
    if (data == nullptr) {
        return;
    }

    memcpy(data, g_audio_buffer->data(), size);
    printf("audioCallback(): size=%d, free=%d\n", size, free);
    int pos = 0;
    while (free > 0 && pos < size) {
        int avail = size - pos;
        if (avail > free)
            avail = free;

        pThis->mAudioOutputEngine->write(data + pos, avail);
        pos += avail;
        free -= avail;
    }

    delete[] data;

    struct VideoPlayerWaitInfo* pwi2 = &pThis->mAudioBufferDoneWaitInfo;
    android::base::AutoLock lock2(pwi2->lock);
    pwi2->done = true;
    pwi2->cvDone.signalAndUnlock(&pwi2->lock);

    printf("audioCallback(): exit\n");
}

void VideoPlayer::stop() {
    printf("VideoPlayer::stop(), mNumVideoFrames=%d, mNumVideoPicts=%d\n",
           mNumVideoFrames, mNumVideoPicts);
    mRunning = false;

    mTimer.stop();
    printf("VideoPlayer::stop() 1\n");
    internalStop();
    printf("VideoPlayer::stop() 2\n");
}

void VideoPlayer::internalStop() {
    mRunning = false;

    if (mAudioQueue != nullptr) {
        mAudioQueue->signalWait();
    }

    if (mVideoQueue != nullptr) {
        mVideoQueue->signalWait();
    }

    struct VideoPlayerWaitInfo* pwi = &mFrameQueueWaitInfo;
    pwi->lock.lock();
    pwi->cvDone.signalAndUnlock(&pwi->lock);

    printf("VideoPlayer::internalStop() 3\n");
}

} // namespace videoplayer
} // namespace android
