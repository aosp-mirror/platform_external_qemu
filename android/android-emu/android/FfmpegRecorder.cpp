// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

//
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
//

#include "android/FfmpegRecorder.h"
#include "android/ffmpeg-recorder.h"

#include "android/audio/AudioCaptureThread.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"
#include "android/utils/debug.h"
#include "android/utils/string.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavutil/avassert.h"
#include "libavutil/channel_layout.h"
#include "libavutil/mathematics.h"
#include "libavutil/opt.h"
#include "libavutil/timestamp.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}

#include <memory>
#include <string>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STREAM_PIX_FMT AV_PIX_FMT_YUV420P /* default pix_fmt */

#define SCALE_FLAGS SWS_BICUBIC

#define DEBUG_VIDEO 0
#define DEBUG_AUDIO 0

#if DEBUG_VIDEO
#include <stdio.h>
#define D_V(...) fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#else
#define D_V(...) (void)0
#endif

#if DEBUG_AUDIO
#include <stdio.h>
#define D_A(...) fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#else
#define D_A(...) (void)0
#endif

namespace android {
namespace recording {

using android::audio::AudioCaptureThread;
using android::base::AutoLock;
using android::base::Lock;

namespace {
// a wrapper around a single output AVStream
struct VideoOutputStream {
    AVStream* stream;

    AVCodecID codecId;

    // video width and height
    int width;
    int height;

    int bitrate;
    int fps;

#if DEBUG_VIDEO
    int64_t frameCount;
    int64_t writeFrameCount;
#endif

    // pts of the next frame that will be generated
    int64_t nextPts;

    AVFrame* frame;
    AVFrame* tmpFrame;

    struct SwsContext* swsCtx;
};

struct AudioOutputStream {
    AVStream* stream;

    AVCodecID codecId;

    int bitrate;
    int sampleRate;

#if DEBUG_AUDIO
    int64_t frameCount;
    int64_t writeFrameCount;
#endif

    // pts of the next frame that will be generated
    int64_t nextPts;

    int samplesCount;
    int nbSamples;

    AVFrame* frame;
    AVFrame* tmpFrame;

    std::unique_ptr<uint8_t> audioLeftover;
    int audioLeftoverLen;

    struct SwrContext* swrCtx;
};

class FfmpegRecorderImpl : public FfmpegRecorder {
public:
    // Ctor
    FfmpegRecorderImpl(int fbWidth, int fbHeight);

    virtual bool initOutputContext(const RecordingInfo* info) override;
    virtual bool start() override;
    virtual bool stop() override;

    virtual bool addAudioTrack(int bitRate, int sampleRate) override;
    virtual bool addVideoTrack(int width,
                               int height,
                               int bitRate,
                               int fps,
                               int intraSpacing) override;

    virtual bool encodeAudioFrame(void* buffer,
                                  int size,
                                  uint64_t ptUs) override;
    virtual bool encodeVideoFrame(const uint8_t* pixels,
                                  int size,
                                  uint64_t ptUs,
                                  RecordPixFmt pixFmt) override;

private:
    // Interleave the packets
    bool writeFrame(AVStream* stream, AVPacket* pkt);

    // Open audio context and allocate audio frames
    bool openAudioContext(AVCodec* codec, AVDictionary* optArgs);
    // Open video context and allocate video frames
    bool openVideoContext(AVCodec* codec, AVDictionary* optArgs);

    void closeAudioContext();
    void closeVideoContext();

    // Write the audio frame
    bool writeAudioFrame(AVFrame* frame);
    // Write the video frame
    bool writeVideoFrame(AVFrame* frame);

    // Reset the state of the recorder
    void reset();

private:
    // Callback for audio capture thread
    static int audioCaptureCallback(void* opaque,
                                    void* buffer,
                                    int size,
                                    uint64_t ptUs);
    // Allocate AVFrame for audio
    static AVFrame* allocAudioFrame(enum AVSampleFormat sampleFmt,
                                    uint64_t channelLayout,
                                    int sampleRate,
                                    int nbSamples);
    // Allocate AVFrame for video
    static AVFrame* allocVideoFrame(enum AVPixelFormat pixFmt,
                                    int width,
                                    int height);
    // FFMpeg defines a set of macros for the same purpose, but those don't
    // compile in C++ mode.
    // Let's define regular C++ functions for it.
    static std::string avTs2Str(int64_t ts);
    static std::string avTs2TimeStr(int64_t ts, AVRational* tb);
    static std::string avErr2Str(int errnum);
    static void logPacket(const AVFormatContext* fmtCtx, const AVPacket* pkt);

    static AVPixelFormat toFfmpegPixFmt(RecordPixFmt r);

public:
    // Gif conversion methods
    static bool convertToAnimatedGif(const char* inFilename,
                                     const char* outFilename,
                                     int bitRate);

private:
    // Gif conversion helpers
    static int gifEncodeWriteFrame(AVFormatContext* ofmt_ctx,
                                   AVFrame* filt_frame,
                                   unsigned int stream_index,
                                   int* got_frame);
    static int gifFlushEncoder(AVFormatContext* ofmt_ctx,
                               unsigned int stream_index);
    static void gifFreeContexts(AVFormatContext* ifmt_ctx,
                                AVFormatContext* ofmt_ctx,
                                int stream_index);

private:
    std::string mPath;
    AVFormatContext* mOutputContext = nullptr;
    VideoOutputStream mVideoStream = {0};
    AudioOutputStream mAudioStream = {0};
    // A single lock to protect writing audio and video frames to the video file
    Lock mInterleaveLock;
    bool mHasAudioTrack = false;
    bool mHasVideoTrack = false;
    bool mHasVideoFrames = false;
    bool mStarted = false;
    uint64_t mStartTimeUs = 0ll;
    int mFbWidth = 0;
    int mFbHeight = 0;
    std::unique_ptr<AudioCaptureThread> mAudioCaptureThread;
};

FfmpegRecorderImpl::FfmpegRecorderImpl(int fbWidth, int fbHeight)
    : mFbWidth(fbWidth), mFbHeight(fbHeight) {
    assert(mFbWidth > 0 && mFbHeight > 0);
}

void FfmpegRecorderImpl::reset() {
    mPath = "";
    mOutputContext = nullptr;
    mVideoStream = {0};
    mAudioStream = {0};
    mStarted = false;
    mHasAudioTrack = false;
    mHasVideoTrack = false;
    mHasVideoFrames = false;
    mStartTimeUs = 0ll;
    mAudioCaptureThread.reset();
}

bool FfmpegRecorderImpl::initOutputContext(const RecordingInfo* info) {
    if (mStarted) {
        derror("%s: Recording already started", __func__);
        return false;
    }
    if (info == nullptr || info->fileName == nullptr) {
        derror("%s: Bad recording info", __func__);
        return false;
    }

    std::string tmpfile = info->fileName;
    std::transform(tmpfile.begin(), tmpfile.end(), tmpfile.begin(), ::tolower);
    if (!str_ends_with(tmpfile.c_str(), ".webm")) {
        derror("%s: %s must have a .webm extension\n", __func__,
               info->fileName);
        return false;
    }

    mPath = info->fileName;

    // Initialize libavcodec, and register all codecs and formats. does not hurt
    // to register multiple times
    av_register_all();

    // allocate the output media context
    avformat_alloc_output_context2(&mOutputContext, nullptr, nullptr,
                                   mPath.c_str());
    if (mOutputContext == nullptr) {
        derror("%s: avformat_alloc_output_context2 failed", __func__);
        return false;
    }

    AVOutputFormat* fmt = mOutputContext->oformat;
    // open the output file, if needed
    if (!(fmt->flags & AVFMT_NOFILE)) {
        int ret =
                avio_open(&mOutputContext->pb, mPath.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            derror("%s: Could not open '%s': %s\n", __func__, mPath.c_str(),
                   avErr2Str(ret).c_str());
            avformat_free_context(mOutputContext);
            return false;
        }
    }

    mAudioStream.codecId = AV_CODEC_ID_VORBIS;
    mVideoStream.codecId = AV_CODEC_ID_VP9;

    return true;
}

bool FfmpegRecorderImpl::start() {
    int ret;
    AVDictionary* opt = nullptr;

    if (mStarted) {
        derror("%s: Muxer already started\n", __func__);
        return false;
    }
    if (mOutputContext == nullptr) {
        derror("%s: Output context was not initialized\n", __func__);
        return false;
    }
    if (!mHasVideoTrack) {
        derror("%s: A video track is required to record\n", __func__);
        return false;
    }

    av_dump_format(mOutputContext, 0, mPath.c_str(), 1);

    // Write the stream header, if any.
    ret = avformat_write_header(mOutputContext, &opt);
    if (ret < 0) {
        derror("Error occurred when opening output file: %s\n",
               avErr2Str(ret).c_str());
        return false;
    }

    mStarted = true;
    mStartTimeUs = android::base::System::get()->getHighResTimeUs();

    return true;
}

bool FfmpegRecorderImpl::stop() {
    if (!mStarted) {
        return false;
    }

    printf("stopping mAudioCaptureThread\n");
    mAudioCaptureThread->stop();
    mAudioCaptureThread->wait();
    printf("Done mAudioCaptureThread\n");

    // flush video encoding with a NULL frame
    while (mHasVideoTrack) {
        AVPacket pkt = {0};
        int gotPacket = 0;
        av_init_packet(&pkt);
        int ret = avcodec_encode_video2(mVideoStream.stream->codec, &pkt,
                                        nullptr, &gotPacket);
        if (ret < 0 || !gotPacket) {
            break;
        }
        D_V("%s: Writing frame %ld\n", __func__,
            mVideoStream.writeFrameCount++);
        writeFrame(mVideoStream.stream, &pkt);
    }

    // flush the remaining audio packet
    if (mHasAudioTrack && (mAudioStream.audioLeftoverLen > 0)) {
        AVFrame* frame = mAudioStream.tmpFrame;
        int frameSize = frame->nb_samples *
                        mAudioStream.stream->codec->channels *
                        sizeof(int16_t);  // 16-bit
        if (mAudioStream.audioLeftoverLen <
            frameSize) {  // this should always true
            int size = frameSize - mAudioStream.audioLeftoverLen;
            std::unique_ptr<uint8_t> zeros(new uint8_t[size]);
            ::memset(zeros.get(), 0, size);
            encodeAudioFrame(zeros.get(), size,
                             android::base::System::get()->getHighResTimeUs());
        }
    }

    // flush audio encoding with a NULL frame
    while (mHasAudioTrack) {
        AVPacket pkt = {0};
        int gotPacket;
        av_init_packet(&pkt);
        int ret = avcodec_encode_audio2(mAudioStream.stream->codec, &pkt, NULL,
                                        &gotPacket);
        if (ret < 0 || !gotPacket) {
            break;
        }
        D_A("%s: Writing audio frame %d\n", __func__,
            mAudioStream.writeFrameCount++);
        writeFrame(mAudioStream.stream, &pkt);
    }

    // Write the trailer, if any. The trailer must be written before you
    // close the CodecContexts open when you wrote the header; otherwise
    // av_write_trailer() may try to use memory that was freed on
    // av_codec_close().
    if (mHasVideoFrames) {
        // This crashes on linux if no frames were encoded.
        av_write_trailer(mOutputContext);
    }

    // Close each codec.
    if (mHasVideoTrack) {
        closeVideoContext();
    }

    if (mHasAudioTrack) {
        closeAudioContext();
    }

    if (mAudioStream.audioLeftover != nullptr) {
        mAudioStream.audioLeftover.reset();
        mAudioStream.audioLeftoverLen = 0;
    }

    // Close the output file.
    if (!(mOutputContext->oformat->flags & AVFMT_NOFILE))
        avio_closep(&mOutputContext->pb);

    // free the stream
    avformat_free_context(mOutputContext);

    bool ret = mHasVideoFrames;
    reset();

    return ret;
}

// static method for audio capture thread callback
int FfmpegRecorderImpl::audioCaptureCallback(void* opaque,
                                             void* buffer,
                                             int size,
                                             uint64_t ptUs) {
    FfmpegRecorderImpl* f = reinterpret_cast<FfmpegRecorderImpl*>(opaque);

    if (f == nullptr) {
        return -1;
    }

    return f->encodeAudioFrame(buffer, size, ptUs) ? 0 : -1;
}

bool FfmpegRecorderImpl::addAudioTrack(int bitRate, int sampleRate) {
    if (mStarted) {
        derror("%s: Muxer already started\n", __func__);
        return false;
    }
    if (mOutputContext == nullptr) {
        derror("%s: Output context was not initialized\n", __func__);
        return false;
    }
    if (mHasAudioTrack) {
        derror("%s: An audio track was already added\n", __func__);
        return false;
    }

    AudioOutputStream* ost = &mAudioStream;
    AVCodecID codecId = ost->codecId;
    AVFormatContext* oc = mOutputContext;
    AVOutputFormat* fmt = oc->oformat;

    fmt->video_codec = mVideoStream.codecId;

    ost->bitrate = bitRate;
    ost->sampleRate = sampleRate;
#if DEBUG_AUDIO
    ost->frameCount = 0;
    ost->writeFrameCount = 0;
#endif

    // find the encoder
    AVCodec* codec = avcodec_find_encoder(codecId);
    if (codec == nullptr) {
        derror("Could not find encoder for '%s'\n", avcodec_get_name(codecId));
        return false;
    }

    ost->stream = avformat_new_stream(oc, codec);
    if (!ost->stream) {
        derror("Could not allocate stream\n");
        return false;
    }

    ost->stream->id = oc->nb_streams - 1;
    AVCodecContext* c = ost->stream->codec;

    if (codec->type != AVMEDIA_TYPE_AUDIO) {
        return false;
    }

    c->sample_fmt =
            codec->sample_fmts ? codec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
    c->bit_rate = bitRate;
    c->sample_rate = sampleRate;

    int i;
    if (codec->supported_samplerates) {
        c->sample_rate = codec->supported_samplerates[0];
        for (i = 0; codec->supported_samplerates[i]; i++) {
            if (codec->supported_samplerates[i] == sampleRate)
                c->sample_rate = sampleRate;
        }
    }
    c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
    c->channel_layout = AV_CH_LAYOUT_STEREO;
    if (codec->channel_layouts) {
        c->channel_layout = codec->channel_layouts[0];
        for (i = 0; codec->channel_layouts[i]; i++) {
            if (codec->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
                c->channel_layout = AV_CH_LAYOUT_STEREO;
        }
    }

    c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
    ost->stream->time_base = (AVRational){1, 1000000 /*c->sample_rate*/};

    // c->time_base = ost->st->time_base;

    VERBOSE_PRINT(
            capture,
            "c->sample_fmt=%d, c->channels=%d, ost->st->time_base.den=%d\n",
            c->sample_fmt, c->channels, ost->stream->time_base.den);

    // Some formats want stream headers to be separate.
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if (codec != nullptr &&
        (codec->capabilities & CODEC_CAP_EXPERIMENTAL) != 0) {
        ost->stream->codec->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    }

    AVDictionary* opt = nullptr;
    if (!openAudioContext(codec, opt)) {
        return false;
    }

    // start audio capture, this informs QEMU to send audio to our muxer
    mAudioCaptureThread.reset(AudioCaptureThread::create(
            FfmpegRecorderImpl::audioCaptureCallback, this, sampleRate,
            ost->frame->nb_samples, 16, 2));
    mAudioCaptureThread->start();

    mHasAudioTrack = true;
    return true;
}

bool FfmpegRecorderImpl::addVideoTrack(int width,
                                       int height,
                                       int bitRate,
                                       int fps,
                                       int intraSpacing) {
    if (mStarted) {
        derror("%s: Muxer already started\n", __func__);
        return false;
    }
    if (mOutputContext == nullptr) {
        derror("%s: Output context was not initialized\n", __func__);
        return false;
    }
    if (mHasVideoTrack) {
        derror("%s: A video track was already added\n", __func__);
        return false;
    }

    VideoOutputStream* ost = &mVideoStream;
    AVCodecID codecId = ost->codecId;
    AVFormatContext* oc = mOutputContext;
    AVOutputFormat* fmt = oc->oformat;

    fmt->video_codec = codecId;

    ost->width = width;
    ost->height = height;
    ost->bitrate = bitRate;
    ost->fps = fps;
#if DEBUG_VIDEO
    ost->frameCount = 0;
    ost->writeFrameCount = 0;
#endif

    // find the encoder
    AVCodec* codec = avcodec_find_encoder(codecId);
    if (codec == nullptr) {
        derror("Could not find encoder for '%s'\n", avcodec_get_name(codecId));
        return false;
    }

    ost->stream = avformat_new_stream(oc, codec);
    if (!ost->stream) {
        derror("Could not allocate stream\n");
        return false;
    }

    ost->stream->id = oc->nb_streams - 1;
    AVCodecContext* c = ost->stream->codec;

    if (codec->type != AVMEDIA_TYPE_VIDEO) {
        return false;
    }

    c->codec_id = codecId;

    c->bit_rate = ost->bitrate;
    // Resolution must be a multiple of two.
    c->width = ost->width;
    c->height = ost->height;
    c->thread_count = 8;

    // timebase: This is the fundamental unit of time (in seconds) in terms
    // of which frame timestamps are represented. For fixed-fps content,
    // timebase should be 1/framerate and timestamp increments should be
    // identical to 1.
    if (codecId == AV_CODEC_ID_VP9) {
        ost->stream->time_base = (AVRational){1, fps};
    } else {
        ost->stream->time_base =
                (AVRational){1, 1000000};  // microsecond timebase
    }
    c->time_base = ost->stream->time_base;

    ost->stream->avg_frame_rate = (AVRational){1, fps};

    c->gop_size =
            intraSpacing;  // emit one intra frame every twelve frames at most
    c->pix_fmt = STREAM_PIX_FMT;
    if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
        // just for testing, we also add B frames
        c->max_b_frames = 2;
    }

    if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
        // Needed to avoid using macroblocks in which some coeffs overflow.
        // This does not happen with normal video, it just happens here as
        // the motion of the chroma plane does not match the luma plane.
        c->mb_decision = 2;
    }

    // Some formats want stream headers to be separate.
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if (codec != nullptr &&
        (codec->capabilities & CODEC_CAP_EXPERIMENTAL) != 0) {
        ost->stream->codec->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    }

    AVDictionary* opt = nullptr;
    if (!openVideoContext(codec, opt)) {
        return false;
    }

    mHasVideoTrack = true;
    return true;
}

bool FfmpegRecorderImpl::encodeAudioFrame(void* buffer,
                                          int size,
                                          uint64_t ptUs) {
    if (!mStarted || !mHasAudioTrack) {
        return false;
    }

    AudioOutputStream* ost = &mAudioStream;
    if (ost->stream == nullptr) {
        return false;
    }

    if (mOutputContext == nullptr) {
        return false;
    }

    AVFrame* frame = ost->tmpFrame;
    int frameSize = frame->nb_samples * ost->stream->codec->channels *
                    sizeof(int16_t);  // 16-bit

    // we need to split into frames
    int remaining = size;
    uint8_t* buf = static_cast<uint8_t*>(buffer);

    if (ost->audioLeftoverLen > 0) {
        if (ost->audioLeftoverLen + size >= frameSize) {
            memcpy(ost->audioLeftover.get() + ost->audioLeftoverLen, buf,
                   frameSize - ost->audioLeftoverLen);
            memcpy(frame->data[0], ost->audioLeftover.get(), frameSize);
            uint64_t elapsedUS = ptUs - mStartTimeUs;
            int64_t pts =
                    (int64_t)(((double)elapsedUS * ost->stream->time_base.den) /
                              1000000.00);
            frame->pts = pts;
            writeAudioFrame(frame);
            buf += (frameSize - ost->audioLeftoverLen);
            remaining -= (frameSize - ost->audioLeftoverLen);
            ost->audioLeftoverLen = 0;
        } else {  // not enough for one frame yet
            memcpy(ost->audioLeftover.get() + ost->audioLeftoverLen, buf, size);
            ost->audioLeftoverLen += size;
            remaining = 0;
        }
    }

    while (remaining >= frameSize) {
        memcpy(frame->data[0], buf, frameSize);
        uint64_t elapsedUS = ptUs - mStartTimeUs;
        int64_t pts = (int64_t)(
                ((double)elapsedUS * ost->stream->time_base.den) / 1000000.00);
        frame->pts = pts;
        writeAudioFrame(frame);
        buf += frameSize;
        remaining -= frameSize;
    }

    if (remaining > 0) {
        if (ost->audioLeftover == nullptr) {
            ost->audioLeftover.reset(new uint8_t[frameSize]);
        }
        memcpy(ost->audioLeftover.get(), buf, remaining);
        ost->audioLeftoverLen = remaining;
    }

    return true;
}

bool FfmpegRecorderImpl::encodeVideoFrame(const uint8_t* pixels,
                                          int size,
                                          uint64_t ptUs,
                                          RecordPixFmt pixFmt) {
    if (!mStarted || !mHasVideoTrack) {
        return false;
    }

    VideoOutputStream* ost = &mVideoStream;
    AVCodecContext* c = ost->stream->codec;

    if (ost->swsCtx == nullptr) {
        AVPixelFormat avPixFmt = toFfmpegPixFmt(pixFmt);
        if (avPixFmt == AV_PIX_FMT_NONE) {
            derror("Pixel format is not supported");
            return false;
        }

        ost->swsCtx = sws_getContext(mFbWidth, mFbHeight, avPixFmt, c->width,
                                     c->height, c->pix_fmt, SCALE_FLAGS, NULL,
                                     NULL, NULL);
        if (ost->swsCtx == nullptr) {
            derror("Could not initialize the conversion context\n");
            return false;
        }
    }

    const int linesize[1] = {FfmpegRecorder::getPixelSize(pixFmt) * mFbWidth};
#if DEBUG_VIDEO
    // To test the speed of sws_scale()
    auto startUs = android::base::System::get()->getHighResTimeUs();
#endif
    sws_scale(ost->swsCtx, (const uint8_t* const*)&pixels, linesize, 0,
              mFbHeight, ost->frame->data, ost->frame->linesize);
#if DEBUG_VIDEO
    D_V("Time to sws_scale: [%lld ms]\n",
        (long long)(android::base::System::get()->getHighResTimeUs() -
                    startUs) /
                1000);
#endif

    uint64_t elapsedUS = ptUs - mStartTimeUs;
    ost->frame->pts = (int64_t)(
            ((double)elapsedUS * ost->stream->time_base.den) / 1000000.00);

    bool ret = writeVideoFrame(ost->frame);
    mHasVideoFrames = true;

    return ret;
}

bool FfmpegRecorderImpl::writeFrame(AVStream* stream, AVPacket* pkt) {
    pkt->dts = AV_NOPTS_VALUE;
    pkt->stream_index = stream->index;

    logPacket(mOutputContext, pkt);

    // Write the compressed frame to the media file.
    AutoLock lock(mInterleaveLock);
    // DO NOT free or unref enc_pkt once interleaved.
    // av_interleaved_write_frame() will take ownership of the packet once
    // passed in.
    return av_interleaved_write_frame(mOutputContext, pkt) == 0;
}

bool FfmpegRecorderImpl::writeAudioFrame(AVFrame* frame) {
    int dstNbSamples;
    int ret;
    AVCodecContext* c = mAudioStream.stream->codec;

    if (frame) {
        // convert samples from native format to destination codec format, using
        // the resampler compute destination number of samples
        dstNbSamples = av_rescale_rnd(
                swr_get_delay(mAudioStream.swrCtx, c->sample_rate) +
                        frame->nb_samples,
                c->sample_rate, c->sample_rate, AV_ROUND_UP);
        av_assert0(dstNbSamples == frame->nb_samples);

        // when we pass a frame to the encoder, it may keep a reference to it
        // internally;
        // make sure we do not overwrite it here
        //
        ret = av_frame_make_writable(mAudioStream.frame);
        if (ret < 0) {
            return false;
        }

        // convert to destination format
        ret = swr_convert(mAudioStream.swrCtx, mAudioStream.frame->data,
                          dstNbSamples, (const uint8_t**)frame->data,
                          frame->nb_samples);
        if (ret < 0) {
            derror("Error while converting\n");
            return false;
        }
        mAudioStream.frame->pts = frame->pts;
        frame = mAudioStream.frame;
        mAudioStream.samplesCount += dstNbSamples;
    }
    // NOTE: If frame is null, then this call is trying to flush out an audio
    // packet from the encoder.

    AVPacket pkt;
    int gotPacket;

    D_A("Encoding audio frame %d\n", mAudioStream.frameCount++);
    av_init_packet(&pkt);
    pkt.data = NULL;  // data and size must be 0
    pkt.size = 0;
    ret = avcodec_encode_audio2(c, &pkt, frame, &gotPacket);
    if (ret < 0) {
        derror("Error encoding audio frame: %s\n", avErr2Str(ret).c_str());
        return false;
    }

    if (gotPacket && pkt.size > 0) {
        D_A("%sWriting audio frame %d\n",
            pkt.flags & AV_PKT_FLAG_KEY ? "(KEY) " : "",
            mAudioStream.writeFrameCount++);
#if DEBUG_AUDIO
        logPacket(mOutputContext, &pkt);
#endif

        if (!writeFrame(mAudioStream.stream, &pkt)) {
            derror("Error while writing audio frame: %s\n",
                   avErr2Str(ret).c_str());
            return false;
        }
    }

    return frame || gotPacket;
}

bool FfmpegRecorderImpl::writeVideoFrame(AVFrame* frame) {
    int ret;
    AVCodecContext* c;
    int gotPacket = 0;

    c = mVideoStream.stream->codec;

    if (mOutputContext->oformat->flags & AVFMT_RAWPICTURE) {
        // a hack to avoid data copy with some raw video muxers
        AVPacket pkt;
        av_init_packet(&pkt);

        if (!frame) {
            return false;
        }

        pkt.flags |= AV_PKT_FLAG_KEY;
        pkt.stream_index = mVideoStream.stream->index;
        pkt.data = (uint8_t*)frame;
        pkt.size = sizeof(AVPicture);

        pkt.pts = pkt.dts = frame->pts;
        av_packet_rescale_ts(&pkt, c->time_base,
                             mVideoStream.stream->time_base);

        {
            AutoLock lock(mInterleaveLock);
            ret = av_interleaved_write_frame(mOutputContext, &pkt);
        }
    } else {
        AVPacket pkt;
        av_init_packet(&pkt);
        pkt.data = nullptr;  // data and size must be 0
        pkt.size = 0;
        // encode the frame
        D_V("Encoding video frame %ld\n", mVideoStream.frameCount++);
#if DEBUG_VIDEO
        auto startUs = android::base::System::get()->getHighResTimeUs();
#endif
        ret = avcodec_encode_video2(c, &pkt, frame, &gotPacket);
        D_V("Time to avcodec_encode_video2: [%lld ms]\n",
            (long long)(android::base::System::get()->getHighResTimeUs() -
                        startUs) /
                    1000);
        if (ret < 0) {
            derror("Error encoding video frame: %s\n", avErr2Str(ret).c_str());
            return false;
        }

        if (gotPacket && pkt.size > 0) {
            D_V("%sWriting frame %ld\n",
                (pkt.flags & AV_PKT_FLAG_KEY) ? "(KEY) " : "",
                mVideoStream.writeFrameCount++);
#if DEBUG_VIDEO
            logPacket(mOutputContext, &pkt);
#endif
            ret = writeFrame(mVideoStream.stream, &pkt) ? 0 : -1;
        } else {
            ret = 0;
        }
    }

    if (ret < 0) {
        derror("Error while writing video frame: %s\n", avErr2Str(ret).c_str());
        return false;
    }

    return frame || gotPacket;
}

// static
AVFrame* FfmpegRecorderImpl::allocAudioFrame(enum AVSampleFormat sampleFmt,
                                             uint64_t channelLayout,
                                             int sampleRate,
                                             int nbSamples) {
    AVFrame* frame = av_frame_alloc();
    int ret;

    if (!frame) {
        derror("Error allocating an audio frame\n");
        return nullptr;
    }

    frame->format = sampleFmt;
    frame->channel_layout = channelLayout;
    frame->sample_rate = sampleRate;
    frame->nb_samples = nbSamples;

    if (nbSamples) {
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            derror("Error allocating an audio buffer\n");
            return nullptr;
        }
    }

    return frame;
}

// static
AVFrame* FfmpegRecorderImpl::allocVideoFrame(enum AVPixelFormat pixFmt,
                                             int width,
                                             int height) {
    AVFrame* picture;
    int ret;

    picture = av_frame_alloc();
    if (!picture) {
        return nullptr;
    }

    picture->format = pixFmt;
    picture->width = width;
    picture->height = height;

    // allocate the buffers for the frame data
    ret = av_frame_get_buffer(picture, 32);
    if (ret < 0) {
        derror("Could not allocate video frame data.\n");
        return nullptr;
    }

    return picture;
}

bool FfmpegRecorderImpl::openAudioContext(AVCodec* codec,
                                          AVDictionary* optArgs) {
    AVCodecContext* c;
    int ret;
    AVDictionary* opts = nullptr;

    c = mAudioStream.stream->codec;

    // open it
    av_dict_copy(&opts, optArgs, 0);

    ret = avcodec_open2(c, codec, &opts);
    av_dict_free(&opts);
    if (ret < 0) {
        derror("Could not open audio codec: %s\n", avErr2Str(ret).c_str());
        return false;
    }

    if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE) {
        mAudioStream.nbSamples = 10000;
    } else {
        mAudioStream.nbSamples = c->frame_size;
    }

    mAudioStream.frame =
            allocAudioFrame(c->sample_fmt, c->channel_layout, c->sample_rate,
                            mAudioStream.nbSamples);
    mAudioStream.tmpFrame =
            allocAudioFrame(AV_SAMPLE_FMT_S16, c->channel_layout,
                            c->sample_rate, mAudioStream.nbSamples);

    // create resampler context
    mAudioStream.swrCtx = swr_alloc();
    if (!mAudioStream.swrCtx) {
        derror("Could not allocate resampler context\n");
        return false;
    }

    // set options
    av_opt_set_int(mAudioStream.swrCtx, "in_channel_count", c->channels, 0);
    av_opt_set_int(mAudioStream.swrCtx, "in_sample_rate", c->sample_rate, 0);
    av_opt_set_sample_fmt(mAudioStream.swrCtx, "in_sample_fmt",
                          AV_SAMPLE_FMT_S16, 0);
    av_opt_set_int(mAudioStream.swrCtx, "out_channel_count", c->channels, 0);
    av_opt_set_int(mAudioStream.swrCtx, "out_sample_rate", c->sample_rate, 0);
    av_opt_set_sample_fmt(mAudioStream.swrCtx, "out_sample_fmt", c->sample_fmt,
                          0);

    // initialize the resampling context
    if ((ret = swr_init(mAudioStream.swrCtx)) < 0) {
        derror("Failed to initialize the resampling context\n");
        return false;
    }

    return true;
}

bool FfmpegRecorderImpl::openVideoContext(AVCodec* codec,
                                          AVDictionary* optArgs) {
    int ret;
    AVCodecContext* c = mVideoStream.stream->codec;
    AVDictionary* opts = nullptr;

    av_dict_copy(&opts, optArgs, 0);

    // vp9
    if (mVideoStream.stream->codec->codec_id == AV_CODEC_ID_VP9) {
        av_dict_set(&opts, "deadline", "realtime" /*"good"*/, 0);
        av_dict_set(&opts, "cpu-used", "8", 0);
    }

    // open the codec
    ret = avcodec_open2(c, codec, &opts);
    av_dict_free(&opts);
    if (ret < 0) {
        derror("Could not open video codec: %s\n", avErr2Str(ret).c_str());
        return false;
    }

    // allocate and init a re-usable frame
    mVideoStream.frame = allocVideoFrame(c->pix_fmt, c->width, c->height);
    if (!mVideoStream.frame) {
        derror("Could not allocate video frame\n");
        return false;
    }

    // If the output format is not YUV420P, then a temporary YUV420P
    // picture is needed too. It is then converted to the required
    // output format.
    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        mVideoStream.tmpFrame =
                allocVideoFrame(AV_PIX_FMT_YUV420P, c->width, c->height);
        if (!mVideoStream.tmpFrame) {
            derror("Could not allocate temporary picture\n");
            return false;
        }
    }

    return true;
}

void FfmpegRecorderImpl::closeAudioContext() {
    avcodec_close(mAudioStream.stream->codec);
    av_frame_free(&mAudioStream.frame);
    av_frame_free(&mAudioStream.tmpFrame);
    swr_free(&mAudioStream.swrCtx);
    mAudioStream.stream = nullptr;
    mAudioStream.frame = nullptr;
    mAudioStream.tmpFrame = nullptr;
    mAudioStream.swrCtx = nullptr;
}

void FfmpegRecorderImpl::closeVideoContext() {
    avcodec_close(mVideoStream.stream->codec);
    av_frame_free(&mVideoStream.frame);
    av_frame_free(&mVideoStream.tmpFrame);
    sws_freeContext(mVideoStream.swsCtx);
    mVideoStream.stream = nullptr;
    mVideoStream.frame = nullptr;
    mVideoStream.tmpFrame = nullptr;
    mVideoStream.swsCtx = nullptr;
}

// static
std::string FfmpegRecorderImpl::avTs2Str(int64_t ts) {
    char res[AV_TS_MAX_STRING_SIZE] = {};
    return av_ts_make_string(res, ts);
}

// static
std::string FfmpegRecorderImpl::avTs2TimeStr(int64_t ts, AVRational* tb) {
    char res[AV_TS_MAX_STRING_SIZE] = {};
    return av_ts_make_time_string(res, ts, tb);
}

// static
std::string FfmpegRecorderImpl::avErr2Str(int errnum) {
    char res[AV_ERROR_MAX_STRING_SIZE] = {};
    return av_make_error_string(res, AV_ERROR_MAX_STRING_SIZE, errnum);
}

// static
AVPixelFormat FfmpegRecorderImpl::toFfmpegPixFmt(RecordPixFmt r) {
    switch (r) {
        case RecordPixFmt::RGB565:
            return AV_PIX_FMT_RGB565;
        case RecordPixFmt::RGBA8888:
            return AV_PIX_FMT_RGBA;
        case RecordPixFmt::BGRA8888:
            return AV_PIX_FMT_BGRA;
        default:
            return AV_PIX_FMT_NONE;
    };
}

// static
void FfmpegRecorderImpl::logPacket(const AVFormatContext* fmtCtx,
                                   const AVPacket* pkt) {
    AVRational* time_base = &fmtCtx->streams[pkt->stream_index]->time_base;

#if DEBUG_VIDEO || DEBUG_AUDIO
    fprintf(stderr,
#else
    VERBOSE_PRINT(
            capture,
#endif
            "pts:%s pts_time:%s dts:%s dts_time:%s duration:%s "
            "duration_time:%s "
            "stream_index:%d\n",
            avTs2Str(pkt->pts).c_str(),
            avTs2TimeStr(pkt->pts, time_base).c_str(),
            avTs2Str(pkt->dts).c_str(),
            avTs2TimeStr(pkt->dts, time_base).c_str(),
            avTs2Str(pkt->duration).c_str(),
            avTs2TimeStr(pkt->duration, time_base).c_str(), pkt->stream_index);
}

// static
bool FfmpegRecorderImpl::convertToAnimatedGif(const char* inFilename,
                                              const char* outFilename,
                                              int bitRate) {
    int ret;
    int video_stream_index = -1;

    // Initialize libavcodec, and register all codecs and formats. does not hurt
    // to register multiple times
    av_register_all();

    // open the input video file
    AVFormatContext* ifmt_ctx = NULL;
    if ((ret = avformat_open_input(&ifmt_ctx, inFilename, NULL, NULL)) < 0) {
        derror("Cannot open input file:%s\n", inFilename);
        return false;
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0) {
        derror("Cannot find stream information\n");
        return false;
    }

    av_dump_format(ifmt_ctx, 0, inFilename, 0);

    // find the video stream, GIF supports only video, no audio
    for (int i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream* stream = ifmt_ctx->streams[i];
        AVCodecContext* codec_ctx = stream->codec;

        if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
            codec_ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
            // Open decoder
            ret = avcodec_open2(
                    codec_ctx, avcodec_find_decoder(codec_ctx->codec_id), NULL);
            if (ret < 0) {
                derror("Failed to open decoder for stream #%u\n", i);
                return false;
            }
            video_stream_index = i;
            break;
        }
    }

    if (video_stream_index == -1) {
        derror("Cannot find video stream\n");
        avformat_close_input(&ifmt_ctx);
        return false;
    }

    // open the output gif file
    AVFormatContext* ofmt_ctx = NULL;
    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, outFilename);
    if (ofmt_ctx == NULL) {
        derror("Could not create output context for %s\n", outFilename);
        avformat_close_input(&ifmt_ctx);
        return false;
    }

    AVStream* out_stream = avformat_new_stream(ofmt_ctx, NULL);
    if (out_stream == NULL) {
        derror("Failed allocating output stream\n");
        gifFreeContexts(ifmt_ctx, ofmt_ctx, video_stream_index);
        return false;
    }

    AVStream* in_stream = ifmt_ctx->streams[video_stream_index];
    AVCodecContext* dec_ctx = in_stream->codec;
    AVCodecContext* enc_ctx = out_stream->codec;

    if (dec_ctx->codec_type != AVMEDIA_TYPE_VIDEO) {
        derror("Failed to find video stream\n");
        gifFreeContexts(ifmt_ctx, ofmt_ctx, video_stream_index);
        return false;
    }

    AVCodec* encoder = avcodec_find_encoder(AV_CODEC_ID_GIF);
    if (encoder == NULL) {
        derror("GIF encoder not found\n");
        gifFreeContexts(ifmt_ctx, ofmt_ctx, video_stream_index);
        return false;
    }

    if ((encoder->capabilities & CODEC_CAP_EXPERIMENTAL) != 0) {
        enc_ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    }

    enc_ctx->height = dec_ctx->height;
    enc_ctx->width = dec_ctx->width;
    enc_ctx->bit_rate = bitRate;
    enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
    // take first format from list of supported formats, which should be
    // AV_PIX_FMT_RGB8
    enc_ctx->pix_fmt = encoder->pix_fmts[0];

    enc_ctx->time_base = dec_ctx->time_base;

    ret = avcodec_open2(enc_ctx, encoder, NULL);
    if (ret < 0) {
        derror("Cannot open video encoder for GIF\n");
        gifFreeContexts(ifmt_ctx, ofmt_ctx, video_stream_index);
        return false;
    }

    if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    av_dump_format(ofmt_ctx, 0, outFilename, 1);

    if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, outFilename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            derror("Could not open output file '%s'", outFilename);
            gifFreeContexts(ifmt_ctx, ofmt_ctx, video_stream_index);
            return false;
        }
    }

    // init muxer, write output file header
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        derror("Error occurred in avformat_write_header()\n");
        gifFreeContexts(ifmt_ctx, ofmt_ctx, video_stream_index);
        return false;
    }

    struct SwsContext* sws_ctx = NULL;
    if (dec_ctx->pix_fmt != enc_ctx->pix_fmt) {
        sws_ctx = sws_getContext(enc_ctx->width, enc_ctx->height,
                                 dec_ctx->pix_fmt, enc_ctx->width,
                                 enc_ctx->height,
                                 /*AV_PIX_FMT_RGBA*/ enc_ctx->pix_fmt,
                                 SCALE_FLAGS, NULL, NULL, NULL);
        if (sws_ctx == NULL) {
            derror("Could not initialize the conversion context\n");
            gifFreeContexts(ifmt_ctx, ofmt_ctx, video_stream_index);
            return false;
        }
    }

    // read all packets, decode, convert, then encode to gif format
    int64_t last_pts = -1;
    int got_frame;
    AVPacket packet = {0};
    packet.data = NULL;
    packet.size = 0;
    while (1) {
        if ((ret = av_read_frame(ifmt_ctx, &packet)) < 0)
            break;
        int stream_index = packet.stream_index;
        if (stream_index != video_stream_index)
            continue;

        // for GIF, it seems we can't use a shared frame, so we have to alloc
        // it here inside the loop
        AVFrame* frame = av_frame_alloc();
        if (!frame) {
            ret = AVERROR(ENOMEM);
            break;
        }

        AVFrame* tmp_frame = allocVideoFrame(enc_ctx->pix_fmt, enc_ctx->width,
                                             enc_ctx->height);
        if (!tmp_frame) {
            av_frame_free(&frame);
            ret = AVERROR(ENOMEM);
            break;
        }

        av_packet_rescale_ts(&packet,
                             ifmt_ctx->streams[stream_index]->time_base,
                             ifmt_ctx->streams[stream_index]->codec->time_base);
        ret = avcodec_decode_video2(ifmt_ctx->streams[stream_index]->codec,
                                    frame, &got_frame, &packet);
        if (ret < 0) {
            av_frame_free(&frame);
            av_frame_free(&tmp_frame);
            derror("Decoding failed\n");
            break;
        }

        ret = 0;
        if (got_frame) {
            frame->pts = av_frame_get_best_effort_timestamp(frame);
            // correct invalid pts in the rare cases

            // Need to check the scaled value if the output time_base has a
            // smaller resolution than the input time_base
            if (ofmt_ctx->streams[stream_index]->codec->time_base.den >
                ofmt_ctx->streams[stream_index]->time_base.den) {
                int64_t last_scaled_pts = -1;
                int64_t scaled_pts = av_rescale_q(
                        frame->pts,
                        ofmt_ctx->streams[stream_index]->codec->time_base,
                        ofmt_ctx->streams[stream_index]->time_base);

                if (last_pts > 0) {
                    last_scaled_pts = av_rescale_q(
                            last_pts,
                            ofmt_ctx->streams[stream_index]->codec->time_base,
                            ofmt_ctx->streams[stream_index]->time_base);
                }
                if (scaled_pts <= last_scaled_pts) {
                    frame->pts = av_rescale_q(
                            last_scaled_pts + 1,
                            ofmt_ctx->streams[stream_index]->time_base,
                            ofmt_ctx->streams[stream_index]->codec->time_base);
                }
            } else {
                if (frame->pts <= last_pts) {
                    frame->pts = last_pts + 1;
                }
            }
            last_pts = frame->pts;
            if (sws_ctx != NULL)
                sws_scale(sws_ctx, frame->data, frame->linesize, 0,
                          enc_ctx->height, tmp_frame->data,
                          tmp_frame->linesize);
            tmp_frame->pts = frame->pts;
            ret = gifEncodeWriteFrame(ofmt_ctx, tmp_frame, stream_index, NULL);
            av_packet_unref(&packet);
            av_frame_free(&frame);
            av_frame_free(&tmp_frame);
            if (ret < 0) {
                break;
            }
        } else {
            av_frame_free(&frame);
            av_frame_free(&tmp_frame);
        }
    }

    if (ret == AVERROR_EOF)
        ret = 0;

    if (ret == 0) {
        // flush encoder
        gifFlushEncoder(ofmt_ctx, video_stream_index);
        av_write_trailer(ofmt_ctx);
    }

    if (sws_ctx != NULL)
        sws_freeContext(sws_ctx);

    gifFreeContexts(ifmt_ctx, ofmt_ctx, video_stream_index);
    return ret == 0;
}
// Gif conversion helper
// encode a frame, when *got_frame returns 1 means that
// the frame is fully encoded. If 0, means more data is
// required to input for encoding.
// static
int FfmpegRecorderImpl::gifEncodeWriteFrame(AVFormatContext* ofmt_ctx,
                                            AVFrame* filt_frame,
                                            unsigned int stream_index,
                                            int* got_frame) {
    int ret;
    int got_frame_local;
    AVPacket enc_pkt;

    if (!got_frame)
        got_frame = &got_frame_local;

    enc_pkt.data = NULL;
    enc_pkt.size = 0;
    av_init_packet(&enc_pkt);
    ret = avcodec_encode_video2(ofmt_ctx->streams[stream_index]->codec,
                                &enc_pkt, filt_frame, got_frame);
    if (ret < 0)
        return ret;
    if (!(*got_frame))
        return 0;

    // prepare packet for muxing
    enc_pkt.stream_index = stream_index;
    av_packet_rescale_ts(&enc_pkt,
                         ofmt_ctx->streams[stream_index]->codec->time_base,
                         ofmt_ctx->streams[stream_index]->time_base);
    // mux encoded frame
    // DO NOT free or unref enc_pkt once interleaved.
    // av_interleaved_write_frame() will take ownership of the packet once
    // passed in.
    ret = av_interleaved_write_frame(ofmt_ctx, &enc_pkt);
    return ret;
}

// Gif conversion helper
// static
int FfmpegRecorderImpl::gifFlushEncoder(AVFormatContext* ofmt_ctx,
                                        unsigned int stream_index) {
    int ret;
    int got_frame;

    if (!(ofmt_ctx->streams[stream_index]->codec->codec->capabilities &
          AV_CODEC_CAP_DELAY))
        return 0;

    while (1) {
        av_log(NULL, AV_LOG_INFO, "Flushing stream #%u encoder\n",
               stream_index);
        ret = gifEncodeWriteFrame(ofmt_ctx, NULL, stream_index, &got_frame);
        if (ret < 0)
            break;
        if (!got_frame)
            return 0;
    }
    return ret;
}

// Gif conversion helper
// static
void FfmpegRecorderImpl::gifFreeContexts(AVFormatContext* ifmt_ctx,
                                         AVFormatContext* ofmt_ctx,
                                         int stream_index) {
    avcodec_close(ifmt_ctx->streams[stream_index]->codec);
    if (ofmt_ctx->streams[stream_index] &&
        ofmt_ctx->streams[stream_index]->codec)
        avcodec_close(ofmt_ctx->streams[stream_index]->codec);

    avformat_close_input(&ifmt_ctx);
    if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);
}

}  // namespace

// FfmpegRecorder static methods
// static
bool FfmpegRecorder::convertToAnimatedGif(const char* inFilename,
                                          const char* outFilename,
                                          int bitRate) {
    return FfmpegRecorderImpl::convertToAnimatedGif(inFilename, outFilename,
                                                    bitRate);
}

// static
int FfmpegRecorder::getPixelSize(RecordPixFmt pixelFormat) {
    switch (pixelFormat) {
        case RecordPixFmt::RGB565:
            return 2;
        case RecordPixFmt::RGBA8888:
        case RecordPixFmt::BGRA8888:
            return 4;
        default:
            return -1;
    }
}

// static
FfmpegRecorder* FfmpegRecorder::create(int fbWidth, int fbHeight) {
    return new FfmpegRecorderImpl(fbWidth, fbHeight);
}

}  // namespace recording
}  // namespace android

// C compatible functions
ffmpeg_recorder ffmpeg_create_recorder(int fb_width, int fb_height) {
    return (ffmpeg_recorder) new android::recording::FfmpegRecorderImpl(
            fb_width, fb_height);
}

bool ffmpeg_recorder_init_output_context(ffmpeg_recorder recorder,
                                         const RecordingInfo* info) {
    android::recording::FfmpegRecorder* r =
            reinterpret_cast<android::recording::FfmpegRecorder*>(recorder);

    if (r != nullptr) {
        return r->initOutputContext(info);
    }
    return false;
}

bool ffmpeg_recorder_start(ffmpeg_recorder recorder) {
    android::recording::FfmpegRecorder* r =
            reinterpret_cast<android::recording::FfmpegRecorder*>(recorder);

    if (r != nullptr) {
        return r->start();
    }
    return false;
}

bool ffmpeg_recorder_stop(ffmpeg_recorder recorder) {
    android::recording::FfmpegRecorder* r =
            reinterpret_cast<android::recording::FfmpegRecorder*>(recorder);

    if (r != nullptr) {
        return r->stop();
    }
    return false;
}

void ffmpeg_delete_recorder(ffmpeg_recorder* recorder) {
    android::recording::FfmpegRecorder** r =
            reinterpret_cast<android::recording::FfmpegRecorder**>(recorder);

    if (r != nullptr && *r != nullptr) {
        delete *r;
        r = nullptr;
    }
}

bool ffmpeg_add_audio_track(ffmpeg_recorder recorder,
                            int bit_rate,
                            int sample_rate) {
    android::recording::FfmpegRecorder* r =
            reinterpret_cast<android::recording::FfmpegRecorder*>(recorder);

    if (r != nullptr) {
        return r->addAudioTrack(bit_rate, sample_rate);
    }
    return false;
}

bool ffmpeg_add_video_track(ffmpeg_recorder recorder,
                            int width,
                            int height,
                            int bit_rate,
                            int fps,
                            int intra_spacing) {
    android::recording::FfmpegRecorder* r =
            reinterpret_cast<android::recording::FfmpegRecorder*>(recorder);

    if (r != nullptr) {
        return r->addVideoTrack(width, height, bit_rate, fps, intra_spacing);
    }
    return false;
}

bool ffmpeg_encode_audio_frame(ffmpeg_recorder recorder,
                               void* buffer,
                               int size,
                               uint64_t ptUs) {
    android::recording::FfmpegRecorder* r =
            reinterpret_cast<android::recording::FfmpegRecorder*>(recorder);

    if (r != nullptr) {
        return r->encodeAudioFrame(buffer, size, ptUs);
    }
    return false;
}

bool ffmpeg_encode_video_frame(ffmpeg_recorder recorder,
                               const uint8_t* rgb_pixels,
                               int size,
                               uint64_t ptUs,
                               RecordPixFmt pixFmt) {
    android::recording::FfmpegRecorder* r =
            reinterpret_cast<android::recording::FfmpegRecorder*>(recorder);

    if (r != nullptr) {
        return r->encodeVideoFrame(rgb_pixels, size, ptUs, pixFmt);
    }
    return false;
}

bool ffmpeg_convert_to_animated_gif(const char* input_video_file,
                                    const char* output_video_file,
                                    int gif_bit_rate) {
    return android::recording::FfmpegRecorder::convertToAnimatedGif(
            input_video_file, output_video_file, gif_bit_rate);
}

int ffmpeg_get_pixel_size(RecordPixFmt r) {
    return android::recording::FfmpegRecorder::getPixelSize(r);
}
