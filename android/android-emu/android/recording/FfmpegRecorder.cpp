// Copyright 2018 The Android Open Source Project
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

#include "android/recording/FfmpegRecorder.h"

#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"
#include "android/recording/Producer.h"
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

// Audio encoding logs
#define D_A(...) VERBOSE_PRINT(ffmpeg_audio, __VA_ARGS__)
// Video encoding logs
#define D_V(...) VERBOSE_PRINT(ffmpeg_video, __VA_ARGS__)

namespace android {
namespace recording {

using android::base::AutoLock;
using android::base::Lock;

namespace {
template <class T>
using CustomUniquePtr = std::unique_ptr<T, std::function<void(T*)>>;

// a wrapper around a single output AVStream
struct VideoOutputStream {
    CustomUniquePtr<AVStream> stream;
    CustomUniquePtr<AVFrame> frame;
    CustomUniquePtr<AVFrame> tmpFrame;
    CustomUniquePtr<SwsContext> swsCtx;

    uint64_t frameCount = 0;
    uint64_t writeFrameCount = 0;

    // pts of the next frame that will be generated
    uint64_t nextPts = 0;

    // video width and height
    uint32_t bitrate = 0;
    AVCodecID codecId = AV_CODEC_ID_NONE;
    uint16_t width = 0;
    uint16_t height = 0;
    uint16_t fps = 0;
};

struct AudioOutputStream {
    CustomUniquePtr<AVStream> stream;
    CustomUniquePtr<AVFrame> frame;
    CustomUniquePtr<AVFrame> tmpFrame;
    CustomUniquePtr<SwrContext> swrCtx;

    std::vector<uint8_t> audioLeftover;

    uint64_t frameCount = 0;
    uint64_t writeFrameCount = 0;

    // pts of the next frame that will be generated
    uint64_t nextPts = 0;

    uint64_t samplesCount = 0;

    uint32_t nbSamples = 0;
    uint32_t bitrate = 0;
    uint32_t sampleRate = 0;
    AVCodecID codecId = AV_CODEC_ID_NONE;
};

class FfmpegRecorderImpl : public FfmpegRecorder {
public:
    // Ctor
    FfmpegRecorderImpl(uint16_t fbWidth,
                       uint16_t fbHeight,
                       android::base::StringView filename);

    virtual ~FfmpegRecorderImpl();

    virtual bool isValid() override;
    virtual bool start() override;
    virtual bool stop() override;

    virtual bool addAudioTrack(uint32_t bitRate, uint32_t sampleRate) override;
    virtual bool addVideoTrack(uint16_t width,
                               uint16_t height,
                               uint32_t bitRate,
                               uint32_t fps,
                               int intraSpacing) override;

    virtual void attachAudioProducer(Producer* producer) override;
    virtual void attachVideoProducer(Producer* producer) override;

private:
    // Initalizes the output context for the muxer. This call is required for
    // adding video/audio contexts and starting the recording.
    bool initOutputContext(android::base::StringView filename);

    // Callbacks to pass to the producers to encode the new av frames.
    bool encodeAudioFrame(const Frame* audioFrame);
    bool encodeVideoFrame(const Frame* videoFrame);

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

    // Abort the recording. This will discard everything and try to shut down as
    // quick as possible. After calling this, the recorder will become invalid.
    void abort();

    // Allocate AVFrame for audio
    static AVFrame* allocAudioFrame(enum AVSampleFormat sampleFmt,
                                    uint64_t channelLayout,
                                    uint32_t sampleRate,
                                    uint32_t nbSamples);
    // Allocate AVFrame for video
    static AVFrame* allocVideoFrame(enum AVPixelFormat pixFmt,
                                    uint16_t width,
                                    uint16_t height);
    // FFMpeg defines a set of macros for the same purpose, but those don't
    // compile in C++ mode.
    // Let's define regular C++ functions for it.
    static std::string avTs2Str(uint64_t ts);
    static std::string avTs2TimeStr(uint64_t ts, AVRational* tb);
    static std::string avErr2Str(int errnum);
    static void logPacket(const AVFormatContext* fmtCtx,
                          const AVPacket* pkt,
                          AVMediaType type);

    static AVPixelFormat toFfmpegPixFmt(VideoFormat r);

private:
    std::string mPath;
    CustomUniquePtr<AVFormatContext> mOutputContext;
    VideoOutputStream mVideoStream;
    AudioOutputStream mAudioStream;
    // A single lock to protect writing audio and video frames to the video file
    Lock mLock;
    bool mHasAudioTrack = false;
    bool mHasVideoTrack = false;
    bool mHasVideoFrames = false;
    bool mStarted = false;
    bool mValid = false;
    uint64_t mStartTimeUs = 0ll;
    uint16_t mFbWidth = 0;
    uint16_t mFbHeight = 0;
    uint8_t mTimeLimit = 0;
    std::unique_ptr<Producer> mAudioProducer;
    std::unique_ptr<Producer> mVideoProducer;
};

FfmpegRecorderImpl::FfmpegRecorderImpl(uint16_t fbWidth,
                                       uint16_t fbHeight,
                                       android::base::StringView filename)
    : mFbWidth(fbWidth), mFbHeight(fbHeight) {
    mValid = initOutputContext(filename);
}

FfmpegRecorderImpl::~FfmpegRecorderImpl() {
    abort();
}

bool FfmpegRecorderImpl::isValid() {
    return mValid;
}

bool FfmpegRecorderImpl::initOutputContext(android::base::StringView filename) {
    if (mStarted) {
        derror("%s: Recording already started", __func__);
        return false;
    }
    if (filename == nullptr) {
        derror("%s: Bad recording info", __func__);
        return false;
    }

    std::string tmpfile = filename;
    std::transform(tmpfile.begin(), tmpfile.end(), tmpfile.begin(), ::tolower);
    if (!str_ends_with(tmpfile.c_str(), ".webm")) {
        derror("%s: %s must have a .webm extension\n", __func__,
               filename.c_str());
        return false;
    }

    mPath = filename;

    // Initialize libavcodec, and register all codecs and formats. does not hurt
    // to register multiple times
    av_register_all();

    // allocate the output media context
    AVFormatContext* outputCtx;
    avformat_alloc_output_context2(&outputCtx, nullptr, nullptr, mPath.c_str());
    if (outputCtx == nullptr) {
        derror("%s: avformat_alloc_output_context2 failed", __func__);
        return false;
    }
    mOutputContext = CustomUniquePtr<AVFormatContext>(
            std::move(outputCtx),
            [](AVFormatContext* ctx) { avformat_free_context(ctx); });

    AVOutputFormat* fmt = mOutputContext->oformat;
    // open the output file, if needed
    if (!(fmt->flags & AVFMT_NOFILE)) {
        int ret =
                avio_open(&mOutputContext->pb, mPath.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            derror("%s: Could not open '%s': %s\n", __func__, mPath.c_str(),
                   avErr2Str(ret).c_str());
            return false;
        }
    }

    mAudioStream.codecId = AV_CODEC_ID_VORBIS;
    mVideoStream.codecId = AV_CODEC_ID_VP9;

    return true;
}

void FfmpegRecorderImpl::attachAudioProducer(Producer* producer) {
    mAudioProducer = std::unique_ptr<Producer>(std::move(producer));
    mAudioProducer->attachCallback([this](const Frame* frame) -> bool {
        return encodeAudioFrame(frame);
    });
}

void FfmpegRecorderImpl::attachVideoProducer(Producer* producer) {
    mVideoProducer = std::unique_ptr<Producer>(std::move(producer));
    mVideoProducer->attachCallback([this](const Frame* frame) -> bool {
        return encodeVideoFrame(frame);
    });
}

bool FfmpegRecorderImpl::start() {
    assert(mValid);

    int ret;
    AVDictionary* opt = nullptr;

    if (mStarted) {
        return true;
    }

    av_dump_format(mOutputContext.get(), 0, mPath.c_str(), 1);

    // Write the stream header, if any.
    ret = avformat_write_header(mOutputContext.get(), &opt);
    if (ret < 0) {
        derror("Error occurred when opening output file: %s\n",
               avErr2Str(ret).c_str());
        return false;
    }

    mStarted = true;
    mStartTimeUs = android::base::System::get()->getHighResTimeUs();

    mVideoProducer->start();
    mAudioProducer->start();

    return true;
}

void FfmpegRecorderImpl::abort() {
    if (!mStarted) {
        return;
    }

    mAudioProducer->stop();
    mVideoProducer->stop();
    mAudioProducer->wait();
    mVideoProducer->wait();
    // Close the output file.
    if (!(mOutputContext->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&mOutputContext->pb);
    }

    mValid = false;
}

bool FfmpegRecorderImpl::stop() {
    assert(mValid);

    if (!mStarted) {
        return false;
    }
    mStarted = false;

    mAudioProducer->stop();
    mVideoProducer->stop();
    mAudioProducer->wait();
    mVideoProducer->wait();

    // flush video encoding with a NULL frame
    if (mHasVideoTrack) {
        while (true) {
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
            writeFrame(mVideoStream.stream.get(), &pkt);
        }
    }

    // flush the remaining audio packet
    if (mHasAudioTrack) {
        if (mAudioStream.audioLeftover.size() > 0) {
            auto frameSize = mAudioStream.audioLeftover.capacity();
            if (mAudioStream.audioLeftover.size() <
                frameSize) {  // this should always true
                auto size = frameSize - mAudioStream.audioLeftover.size();
                auto tsUs = android::base::System::get()->getHighResTimeUs();
                Frame f(size, 0);
                f.tsUs = tsUs;
                f.audioFormat = AudioFormat::AUD_FMT_S16;
                encodeAudioFrame(&f);
            }
        }

        // flush audio encoding with a NULL frame
        while (true) {
            AVPacket pkt = {0};
            int gotPacket;
            av_init_packet(&pkt);
            int ret = avcodec_encode_audio2(mAudioStream.stream->codec, &pkt,
                                            NULL, &gotPacket);
            if (ret < 0 || !gotPacket) {
                break;
            }
            D_A("%s: Writing audio frame %d\n", __func__,
                mAudioStream.writeFrameCount++);
            writeFrame(mAudioStream.stream.get(), &pkt);
        }
    }

    // Write the trailer, if any. The trailer must be written before you
    // close the CodecContexts open when you wrote the header; otherwise
    // av_write_trailer() may try to use memory that was freed on
    // av_codec_close().
    if (mHasVideoFrames) {
        // This crashes on linux if no frames were encoded.
        av_write_trailer(mOutputContext.get());
    }

    // Close each codec.
    if (mHasVideoTrack) {
        closeVideoContext();
    }

    if (mHasAudioTrack) {
        closeAudioContext();
    }

    // Close the output file.
    if (!(mOutputContext->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&mOutputContext->pb);
    }

    // free the stream
    mOutputContext.reset();

    mValid = false;

    // The recording is only valid if we have encoded at least one frame in the
    // recording.
    return mHasVideoFrames;
}

bool FfmpegRecorderImpl::addAudioTrack(uint32_t bitRate, uint32_t sampleRate) {
    assert(mValid);
    if (mStarted) {
        derror("%s: Muxer already started\n", __func__);
        return false;
    }
    if (mHasAudioTrack) {
        derror("%s: An audio track was already added\n", __func__);
        return false;
    }
    D_A("%s(bitRate=[%u], sampleRate=[%u])\n", __func__, bitRate, sampleRate);

    AudioOutputStream* ost = &mAudioStream;
    AVCodecID codecId = ost->codecId;
    AVFormatContext* oc = mOutputContext.get();
    AVOutputFormat* fmt = oc->oformat;

    fmt->video_codec = mVideoStream.codecId;

    ost->bitrate = bitRate;
    ost->sampleRate = sampleRate;
    ost->frameCount = 0;
    ost->writeFrameCount = 0;

    // find the encoder
    AVCodec* codec = avcodec_find_encoder(codecId);
    if (codec == nullptr) {
        derror("Could not find encoder for '%s'\n", avcodec_get_name(codecId));
        return false;
    }

    auto stream = avformat_new_stream(oc, codec);
    if (!stream) {
        derror("Could not allocate stream\n");
        return false;
    }
    ost->stream = CustomUniquePtr<AVStream>(
            std::move(stream), [](AVStream* st) { avcodec_close(st->codec); });

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

    D_A("c->sample_fmt=%d, c->channels=%d, ost->st->time_base.den=%d\n",
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

    auto frameSize = ost->frame->nb_samples * ost->stream->codec->channels *
                     getAudioFormatSize(AudioFormat::AUD_FMT_S16);
    ost->audioLeftover.reserve(frameSize);

    mHasAudioTrack = true;
    return true;
}

bool FfmpegRecorderImpl::addVideoTrack(uint16_t width,
                                       uint16_t height,
                                       uint32_t bitRate,
                                       uint32_t fps,
                                       int intraSpacing) {
    assert(mValid);
    if (mStarted) {
        derror("%s: Muxer already started\n", __func__);
        return false;
    }
    if (mHasVideoTrack) {
        derror("%s: A video track was already added\n", __func__);
        return false;
    }
    D_V("%s(width=[%u], height=[%u], bitRate=[%u], fps=[%u], "
        "intraSpacing=[%d])\n",
        __func__, width, height, bitRate, fps, intraSpacing);

    VideoOutputStream* ost = &mVideoStream;
    AVCodecID codecId = ost->codecId;
    AVFormatContext* oc = mOutputContext.get();
    AVOutputFormat* fmt = oc->oformat;

    fmt->video_codec = codecId;

    ost->width = width;
    ost->height = height;
    ost->bitrate = bitRate;
    ost->fps = fps;
    ost->frameCount = 0;
    ost->writeFrameCount = 0;

    // find the encoder
    AVCodec* codec = avcodec_find_encoder(codecId);
    if (codec == nullptr) {
        derror("Could not find encoder for '%s'\n", avcodec_get_name(codecId));
        return false;
    }

    auto stream = avformat_new_stream(oc, codec);
    if (!stream) {
        derror("Could not allocate stream\n");
        return false;
    }
    ost->stream = CustomUniquePtr<AVStream>(
            std::move(stream), [](AVStream* st) { avcodec_close(st->codec); });

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
        ost->stream->time_base = (AVRational){1, static_cast<int>(fps)};
    } else {
        ost->stream->time_base =
                (AVRational){1, 1000000};  // microsecond timebase
    }
    c->time_base = ost->stream->time_base;

    ost->stream->avg_frame_rate = (AVRational){1, static_cast<int>(fps)};

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

bool FfmpegRecorderImpl::encodeAudioFrame(const Frame* frame) {
    assert(mValid);
    if (!mHasAudioTrack || !frame) {
        return false;
    }

    AudioOutputStream* ost = &mAudioStream;
    if (ost->stream == nullptr) {
        return false;
    }

    AVFrame* avframe = ost->tmpFrame.get();
    auto frameSize = ost->audioLeftover.capacity();

    // we need to split into frames
    auto remaining = frame->dataVec.size();
    auto buf = static_cast<const uint8_t*>(frame->dataVec.data());

    // If we have leftover data, fill the rest of the frame and send to the
    // encoder.
    if (ost->audioLeftover.size() > 0) {
        if (ost->audioLeftover.size() + remaining >= frameSize) {
            auto bufUsed = frameSize - ost->audioLeftover.size();
            ost->audioLeftover.insert(ost->audioLeftover.end(), buf,
                                      buf + bufUsed);
            memcpy(avframe->data[0], ost->audioLeftover.data(), frameSize);
            uint64_t elapsedUS = frame->tsUs - mStartTimeUs;
            int64_t pts =
                    (int64_t)(((double)elapsedUS * ost->stream->time_base.den) /
                              1000000.00);
            avframe->pts = pts;
            writeAudioFrame(avframe);
            buf += bufUsed;
            remaining -= bufUsed;
            ost->audioLeftover.clear();
        } else {  // not enough for one frame yet
            ost->audioLeftover.insert(ost->audioLeftover.end(), buf,
                                      buf + remaining);
            remaining = 0;
        }
    }

    while (remaining >= frameSize) {
        memcpy(avframe->data[0], buf, frameSize);
        uint64_t elapsedUS = frame->tsUs - mStartTimeUs;
        int64_t pts = (int64_t)(
                ((double)elapsedUS * ost->stream->time_base.den) / 1000000.00);
        avframe->pts = pts;
        writeAudioFrame(avframe);
        buf += frameSize;
        remaining -= frameSize;
    }

    if (remaining > 0) {
        ost->audioLeftover.assign(buf, buf + remaining);
    }

    return true;
}

bool FfmpegRecorderImpl::encodeVideoFrame(const Frame* frame) {
    assert(mValid);
    if (!mHasVideoTrack) {
        return false;
    }

    VideoOutputStream* ost = &mVideoStream;
    AVCodecContext* c = ost->stream->codec;

    if (ost->swsCtx == nullptr) {
        AVPixelFormat avPixFmt = toFfmpegPixFmt(frame->videoFormat);
        if (avPixFmt == AV_PIX_FMT_NONE) {
            derror("Pixel format is not supported");
            return false;
        }

        auto swsCtx = sws_getContext(mFbWidth, mFbHeight, avPixFmt, c->width,
                                     c->height, c->pix_fmt, SCALE_FLAGS, NULL,
                                     NULL, NULL);
        if (swsCtx == nullptr) {
            derror("Could not initialize the conversion context\n");
            return false;
        }
        ost->swsCtx = CustomUniquePtr<SwsContext>(
                std::move(swsCtx),
                [](SwsContext* ctx) { sws_freeContext(ctx); });
    }

    const int linesize[1] = {getVideoFormatSize(frame->videoFormat) * mFbWidth};

    // To test the speed of sws_scale()
    auto startUs = android::base::System::get()->getHighResTimeUs();
    auto data = frame->dataVec.data();
    sws_scale(ost->swsCtx.get(), (const uint8_t* const*)&data, linesize, 0,
              mFbHeight, ost->frame->data, ost->frame->linesize);
    D_V("Time to sws_scale: [%lld ms]\n",
        (long long)(android::base::System::get()->getHighResTimeUs() -
                    startUs) /
                1000);

    uint64_t elapsedUS = frame->tsUs - mStartTimeUs;
    ost->frame->pts = (int64_t)(
            ((double)elapsedUS * ost->stream->time_base.den) / 1000000.00);

    bool ret = writeVideoFrame(ost->frame.get());
    mHasVideoFrames = true;

    return ret;
}

bool FfmpegRecorderImpl::writeFrame(AVStream* stream, AVPacket* pkt) {
    pkt->dts = AV_NOPTS_VALUE;
    pkt->stream_index = stream->index;

    logPacket(mOutputContext.get(), pkt, stream->codec->codec->type);

    // Write the compressed frame to the media file.
    AutoLock lock(mLock);
    // DO NOT free or unref enc_pkt once interleaved.
    // av_interleaved_write_frame() will take ownership of the packet once
    // passed in.
    return av_interleaved_write_frame(mOutputContext.get(), pkt) == 0;
}

bool FfmpegRecorderImpl::writeAudioFrame(AVFrame* frame) {
    int dstNbSamples;
    int ret;
    AVCodecContext* c = mAudioStream.stream->codec;

    if (frame) {
        // convert samples from native format to destination codec format, using
        // the resampler compute destination number of samples
        dstNbSamples = av_rescale_rnd(
                swr_get_delay(mAudioStream.swrCtx.get(), c->sample_rate) +
                        frame->nb_samples,
                c->sample_rate, c->sample_rate, AV_ROUND_UP);
        av_assert0(dstNbSamples == frame->nb_samples);

        // when we pass a frame to the encoder, it may keep a reference to it
        // internally;
        // make sure we do not overwrite it here
        //
        ret = av_frame_make_writable(mAudioStream.frame.get());
        if (ret < 0) {
            return false;
        }

        // convert to destination format
        ret = swr_convert(mAudioStream.swrCtx.get(), mAudioStream.frame->data,
                          dstNbSamples, (const uint8_t**)frame->data,
                          frame->nb_samples);
        if (ret < 0) {
            derror("Error while converting\n");
            return false;
        }
        mAudioStream.frame->pts = frame->pts;
        frame = mAudioStream.frame.get();
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
        if (!writeFrame(mAudioStream.stream.get(), &pkt)) {
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
            AutoLock lock(mLock);
            ret = av_interleaved_write_frame(mOutputContext.get(), &pkt);
        }
    } else {
        AVPacket pkt;
        av_init_packet(&pkt);
        pkt.data = nullptr;  // data and size must be 0
        pkt.size = 0;
        // encode the frame
        D_V("Encoding video frame %ld\n", mVideoStream.frameCount++);
        auto startUs = android::base::System::get()->getHighResTimeUs();
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
            ret = writeFrame(mVideoStream.stream.get(), &pkt) ? 0 : -1;
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
                                             uint32_t sampleRate,
                                             uint32_t nbSamples) {
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
                                             uint16_t width,
                                             uint16_t height) {
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

    auto frame = allocAudioFrame(c->sample_fmt, c->channel_layout,
                                 c->sample_rate, mAudioStream.nbSamples);
    if (!frame) {
        derror("Could not allocate AVFrame for audio\n");
        return false;
    }
    mAudioStream.frame = CustomUniquePtr<AVFrame>(
            std::move(frame), [](AVFrame* f) { av_frame_free(&f); });

    frame = allocAudioFrame(AV_SAMPLE_FMT_S16, c->channel_layout,
                            c->sample_rate, mAudioStream.nbSamples);
    if (!frame) {
        derror("Could not allocate AVFrame for audio\n");
        return false;
    }
    mAudioStream.tmpFrame = CustomUniquePtr<AVFrame>(
            std::move(frame), [](AVFrame* f) { av_frame_free(&f); });

    // create resampler context
    auto swrCtx = swr_alloc();
    if (!swrCtx) {
        derror("Could not allocate resampler context\n");
        return false;
    }
    mAudioStream.swrCtx = CustomUniquePtr<SwrContext>(
            std::move(swrCtx), [](SwrContext* ctx) { swr_free(&ctx); });

    // set options
    av_opt_set_int(mAudioStream.swrCtx.get(), "in_channel_count", c->channels,
                   0);
    av_opt_set_int(mAudioStream.swrCtx.get(), "in_sample_rate", c->sample_rate,
                   0);
    av_opt_set_sample_fmt(mAudioStream.swrCtx.get(), "in_sample_fmt",
                          AV_SAMPLE_FMT_S16, 0);
    av_opt_set_int(mAudioStream.swrCtx.get(), "out_channel_count", c->channels,
                   0);
    av_opt_set_int(mAudioStream.swrCtx.get(), "out_sample_rate", c->sample_rate,
                   0);
    av_opt_set_sample_fmt(mAudioStream.swrCtx.get(), "out_sample_fmt",
                          c->sample_fmt, 0);

    // initialize the resampling context
    if ((ret = swr_init(mAudioStream.swrCtx.get())) < 0) {
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
    auto frame = allocVideoFrame(c->pix_fmt, c->width, c->height);
    if (!frame) {
        derror("Could not allocate video frame\n");
        return false;
    }
    mVideoStream.frame = CustomUniquePtr<AVFrame>(
            std::move(frame), [](AVFrame* f) { av_frame_free(&f); });

    // If the output format is not YUV420P, then a temporary YUV420P
    // picture is needed too. It is then converted to the required
    // output format.
    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        auto tmpFrame =
                allocVideoFrame(AV_PIX_FMT_YUV420P, c->width, c->height);
        if (!tmpFrame) {
            derror("Could not allocate temporary picture\n");
            return false;
        }
        mVideoStream.tmpFrame = CustomUniquePtr<AVFrame>(
                std::move(tmpFrame), [](AVFrame* f) { av_frame_free(&f); });
    }

    return true;
}

void FfmpegRecorderImpl::closeAudioContext() {
    mAudioStream.stream.reset();
    mAudioStream.frame.reset();
    mAudioStream.tmpFrame.reset();
    mAudioStream.swrCtx.reset();
}

void FfmpegRecorderImpl::closeVideoContext() {
    mVideoStream.stream.reset();
    mVideoStream.frame.reset();
    mVideoStream.tmpFrame.reset();
    mVideoStream.swsCtx.reset();
}

// static
std::string FfmpegRecorderImpl::avTs2Str(uint64_t ts) {
    char res[AV_TS_MAX_STRING_SIZE] = {};
    return av_ts_make_string(res, ts);
}

// static
std::string FfmpegRecorderImpl::avTs2TimeStr(uint64_t ts, AVRational* tb) {
    char res[AV_TS_MAX_STRING_SIZE] = {};
    return av_ts_make_time_string(res, ts, tb);
}

// static
std::string FfmpegRecorderImpl::avErr2Str(int errnum) {
    char res[AV_ERROR_MAX_STRING_SIZE] = {};
    return av_make_error_string(res, AV_ERROR_MAX_STRING_SIZE, errnum);
}

// static
AVPixelFormat FfmpegRecorderImpl::toFfmpegPixFmt(VideoFormat r) {
    switch (r) {
        case VideoFormat::RGB565:
            return AV_PIX_FMT_RGB565;
        case VideoFormat::RGBA8888:
            return AV_PIX_FMT_RGBA;
        case VideoFormat::BGRA8888:
            return AV_PIX_FMT_BGRA;
        default:
            return AV_PIX_FMT_NONE;
    };
}

// static
void FfmpegRecorderImpl::logPacket(const AVFormatContext* fmtCtx,
                                   const AVPacket* pkt,
                                   AVMediaType type) {
    AVRational* time_base = &fmtCtx->streams[pkt->stream_index]->time_base;
    if ((type == AVMEDIA_TYPE_AUDIO && VERBOSE_CHECK(ffmpeg_audio)) ||
        (type == AVMEDIA_TYPE_VIDEO && VERBOSE_CHECK(ffmpeg_video))) {
        printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s "
               "duration_time:%s "
               "stream_index:%d\n",
               avTs2Str(pkt->pts).c_str(),
               avTs2TimeStr(pkt->pts, time_base).c_str(),
               avTs2Str(pkt->dts).c_str(),
               avTs2TimeStr(pkt->dts, time_base).c_str(),
               avTs2Str(pkt->duration).c_str(),
               avTs2TimeStr(pkt->duration, time_base).c_str(),
               pkt->stream_index);
    }
}
}  // namespace

// static
FfmpegRecorder* FfmpegRecorder::create(uint16_t fbWidth,
                                       uint16_t fbHeight,
                                       android::base::StringView filename) {
    return new FfmpegRecorderImpl(fbWidth, fbHeight, filename);
}

}  // namespace recording
}  // namespace android
