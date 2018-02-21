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

#include "android/base/Log.h"
#include "android/base/files/PathUtils.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"
#include "android/recording/AVScopedPtr.h"
#include "android/recording/Producer.h"
#include "android/utils/debug.h"

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

#include <string>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define D(...) VERBOSE_PRINT(record, __VA_ARGS__)

namespace android {
namespace recording {

using android::base::AutoLock;
using android::base::Lock;
using android::base::PathUtils;

namespace {

// a wrapper around a single output AVStream
struct VideoOutputStream {
    AVStream* stream = nullptr;
    AVScopedPtr<AVCodecContext> codecCtx;
    AVScopedPtr<AVFrame> frame;
    AVScopedPtr<AVFrame> tmpFrame;
    AVScopedPtr<SwsContext> swsCtx;

    uint64_t frameCount = 0;
    uint64_t writeFrameCount = 0;
};

struct AudioOutputStream {
    AVStream* stream = nullptr;
    AVScopedPtr<AVCodecContext> codecCtx;
    AVScopedPtr<AVFrame> frame;
    AVScopedPtr<AVFrame> tmpFrame;
    AVScopedPtr<SwrContext> swrCtx;

    std::vector<uint8_t> audioLeftover;

    uint64_t frameCount = 0;
    uint64_t writeFrameCount = 0;
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

    virtual bool addAudioTrack(
            std::unique_ptr<Producer> producer,
            const Codec<SwrContext>* codec) override;
    virtual bool addVideoTrack(
            std::unique_ptr<Producer> producer,
            const Codec<SwsContext*>* codec) override;

private:
    // Initalizes the output context for the muxer. This call is required for
    // adding video/audio contexts and starting the recording.
    bool initOutputContext(android::base::StringView filename);

    void attachAudioProducer(std::unique_ptr<Producer> producer);
    void attachVideoProducer(std::unique_ptr<Producer> producer);

    // Callbacks to pass to the producers to encode the new av frames.
    bool encodeAudioFrame(const Frame* audioFrame);
    bool encodeVideoFrame(const Frame* videoFrame);

    // Interleave the packets
    bool writeFrame(AVStream* stream, AVPacket* pkt);

    // Open audio context and allocate audio frames
    bool openAudioContext(AVCodec* codec, AVDictionary* optArgs);

    void closeAudioContext();
    void closeVideoContext();

    // Write the audio frame
    bool writeAudioFrame(AVFrame* frame);
    // Write the video frame
    bool writeVideoFrame(AVFrame* frame);

    // Abort the recording. This will discard everything and try to shut down as
    // quick as possible. After calling this, the recorder will become invalid.
    void abortRecording();

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

private:
    std::string mEncodedOutputPath;
    AVScopedPtr<AVFormatContext> mOutputContext;
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
    abortRecording();
}

bool FfmpegRecorderImpl::isValid() {
    return mValid;
}

bool FfmpegRecorderImpl::initOutputContext(android::base::StringView filename) {
    if (mStarted) {
        LOG(ERROR) << __func__ << ": Recording already started";
        return false;
    }
    if (filename.empty()) {
        LOG(ERROR) << __func__ << ": No output filename supplied";
        return false;
    }

    std::string tmpfile = filename;
    std::transform(tmpfile.begin(), tmpfile.end(), tmpfile.begin(), ::tolower);
    if (PathUtils::extension(tmpfile) != ".webm") {
        LOG(ERROR) << __func__ << ": [" << filename
                   << "] must have a .webm extension";
        return false;
    }

    mEncodedOutputPath = filename;

    // Initialize libavcodec, and register all codecs and formats. does not hurt
    // to register multiple times
    av_register_all();

    // allocate the output media context
    AVFormatContext* outputCtx;
    avformat_alloc_output_context2(&outputCtx, nullptr, nullptr,
                                   mEncodedOutputPath.c_str());
    if (outputCtx == nullptr) {
        LOG(ERROR) << __func__ << ": avformat_alloc_output_context2 failed";
        return false;
    }
    mOutputContext = makeAVScopedPtr(outputCtx);

    AVOutputFormat* fmt = mOutputContext->oformat;
    // open the output file, if needed
    if (!(fmt->flags & AVFMT_NOFILE)) {
        int ret = avio_open(&mOutputContext->pb, mEncodedOutputPath.c_str(),
                            AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOG(ERROR) << __func__ << ": Could not open [" << mEncodedOutputPath
                       << "]: ";
            avErr2Str(ret);
            return false;
        }
    }

    return true;
}

void FfmpegRecorderImpl::attachAudioProducer(
        std::unique_ptr<Producer> producer) {
    mAudioProducer = std::move(producer);
    mAudioProducer->attachCallback([this](const Frame* frame) -> bool {
        return encodeAudioFrame(frame);
    });
}

void FfmpegRecorderImpl::attachVideoProducer(
        std::unique_ptr<Producer> producer) {
    mVideoProducer = std::move(producer);
    mVideoProducer->attachCallback([this](const Frame* frame) -> bool {
        return encodeVideoFrame(frame);
    });
}

bool FfmpegRecorderImpl::start() {
    assert(mValid && mHasVideoTrack);

    int ret;
    AVDictionary* opt = nullptr;

    if (mStarted) {
        return true;
    }

    av_dump_format(mOutputContext.get(), 0, mEncodedOutputPath.c_str(), 1);

    // Write the stream header, if any.
    ret = avformat_write_header(mOutputContext.get(), &opt);
    if (ret < 0) {
        LOG(ERROR) << "Error occurred when opening output file: ["
                   << avErr2Str(ret) << "]";
        return false;
    }

    mStarted = true;
    mStartTimeUs = android::base::System::get()->getHighResTimeUs();

    mVideoProducer->start();
    if (mHasAudioTrack) {
        // The audio track add may have failed, so don't start
        // the producer if so.
        mAudioProducer->start();
    }

    return true;
}

void FfmpegRecorderImpl::abortRecording() {
    if (!mStarted) {
        return;
    }

    mAudioProducer->stop();
    mVideoProducer->stop();
    mAudioProducer->wait();
    mVideoProducer->wait();

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
            D("%s: Writing frame %ld\n", __func__,
              mVideoStream.writeFrameCount++);
            writeFrame(mVideoStream.stream, &pkt);
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
                f.format.audioFormat = mAudioProducer->getFormat().audioFormat;
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
            D("%s: Writing audio frame %d\n", __func__,
              mAudioStream.writeFrameCount++);
            writeFrame(mAudioStream.stream, &pkt);
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

    // Close the output file and free the stream
    mOutputContext.reset();

    mValid = false;

    // The recording is only valid if we have encoded at least one frame in the
    // recording.
    return mHasVideoFrames;
}

bool FfmpegRecorderImpl::addAudioTrack(
        std::unique_ptr<Producer> producer,
        const Codec<SwrContext>* codec) {
    assert(mValid);
    if (mStarted) {
        LOG(ERROR) << __func__ << ": Muxer already started";
        return false;
    }
    if (mHasAudioTrack) {
        LOG(ERROR) << __func__ << ": An audio track was already added";
        return false;
    }

    if (codec->getCodec()->type != AVMEDIA_TYPE_AUDIO) {
        LOG(ERROR) << __func__ << " "
                   << avcodec_get_name(codec->getCodecId())
                   << " is not an audio codec.";
        return false;
    }

    auto params = codec->getCodecParams();
    D("%s(bitrate=[%u], sample_rate=[%u])\n", __func__, params->bitrate,
      params->sample_rate);

    attachAudioProducer(std::move(producer));

    AudioOutputStream* ost = &mAudioStream;
    AVFormatContext* oc = mOutputContext.get();

    ost->frameCount = 0;
    ost->writeFrameCount = 0;

    auto stream = avformat_new_stream(oc, codec->getCodec());
    if (!stream) {
        LOG(ERROR) << "Could not allocate audio stream";
        return false;
    }
    ost->stream = stream;

    if (!codec->configAndOpenEncoder(oc, ost->stream)) {
        LOG(ERROR) << "Unable to open video codec context ["
                   << avcodec_get_name(codec->getCodecId()) << "]";
        return false;
    }
    AVCodecContext* c = ost->stream->codec;
    ost->codecCtx = makeAVScopedPtr(c);

    D("%s: c->sample_fmt=%d, c->channels=%d, ost->st->time_base.den=%d\n",
      __func__, c->sample_fmt, c->channels, ost->stream->time_base.den);

    uint32_t nbSamples;
    if (codec->getCodec()->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE) {
        nbSamples = 10000;
    } else {
        nbSamples = c->frame_size;
    }

    // Allocate two frames for resampling
    auto frame = allocAudioFrame(c->sample_fmt, c->channel_layout,
                                 c->sample_rate, nbSamples);
    if (!frame) {
        LOG(ERROR) << "Could not allocate AVFrame for audio";
        return false;
    }
    mAudioStream.frame = makeAVScopedPtr(frame);

    auto inFFSampleFmt =
            toAVSampleFormat(mAudioProducer->getFormat().audioFormat);
    frame = allocAudioFrame(inFFSampleFmt, c->channel_layout, c->sample_rate,
                            nbSamples);
    if (!frame) {
        LOG(ERROR) << "Could not allocate AVFrame for audio";
        return false;
    }
    mAudioStream.tmpFrame = makeAVScopedPtr(frame);

    // create resampler context
    auto swrCtx = swr_alloc();
    if (!swrCtx) {
        LOG(ERROR) << "Could not allocate resampler context";
        return false;
    }
    mAudioStream.swrCtx = makeAVScopedPtr(swrCtx);

    if (!codec->initSwxContext(c, mAudioStream.swrCtx.get())) {
        LOG(ERROR) << "Unable to initialize resampling context";
        return false;
    }

    // Setup frame for leftover audio data
    auto frameSize =
            ost->frame->nb_samples * ost->stream->codec->channels *
            getAudioFormatSize(mAudioProducer->getFormat().audioFormat);
    ost->audioLeftover.reserve(frameSize);

    mHasAudioTrack = true;
    return true;
}

bool FfmpegRecorderImpl::addVideoTrack(
        std::unique_ptr<Producer> producer,
        const Codec<SwsContext*>* codec) {
    assert(mValid);
    if (mStarted) {
        LOG(ERROR) << __func__ << ": Muxer already started";
        return false;
    }
    if (mHasVideoTrack) {
        LOG(ERROR) << __func__ << ": A video track was already added";
        return false;
    }

    if (codec->getCodec()->type != AVMEDIA_TYPE_VIDEO) {
        LOG(ERROR) << __func__ << " "
                   << avcodec_get_name(codec->getCodecId())
                   << " is not a video codec.";
        return false;
    }

    attachVideoProducer(std::move(producer));

    VideoOutputStream* ost = &mVideoStream;
    AVFormatContext* oc = mOutputContext.get();

    auto codecParams = codec->getCodecParams();
    D("%s(width=[%u], height=[%u], bitRate=[%u], fps=[%u], "
      "intraSpacing=[%d])\n",
      __func__, codecParams->width, codecParams->height, codecParams->bitrate,
      codecParams->fps, codecParams->intra_spacing);

    ost->frameCount = 0;
    ost->writeFrameCount = 0;

    auto stream = avformat_new_stream(oc, codec->getCodec());
    if (!stream) {
        LOG(ERROR) << "Could not allocate video stream";
        return false;
    }
    ost->stream = stream;

    if (!codec->configAndOpenEncoder(oc, ost->stream)) {
        LOG(ERROR) << "Unable to open video codec context ["
                   << avcodec_get_name(codec->getCodecId()) << "]";
        return false;
    }
    AVCodecContext* c = ost->stream->codec;
    ost->codecCtx = makeAVScopedPtr(c);

    // allocate and init a re-usable frame
    auto frame = allocVideoFrame(c->pix_fmt, c->width, c->height);
    if (!frame) {
        LOG(ERROR) << "Could not allocate video frame";
        return false;
    }
    mVideoStream.frame = makeAVScopedPtr(frame);

    // If the output format is not YUV420P, then a temporary YUV420P
    // picture is needed too. It is then converted to the required
    // output format.
    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        auto tmpFrame =
                allocVideoFrame(AV_PIX_FMT_YUV420P, c->width, c->height);
        if (!tmpFrame) {
            LOG(ERROR) << "Could not allocate temporary picture";
            return false;
        }
        mVideoStream.tmpFrame = makeAVScopedPtr(tmpFrame);
    }

    // Initialize the rescaling context
    SwsContext* swsCtx = nullptr;
    if (!codec->initSwxContext(c, &swsCtx)) {
        LOG(ERROR) << "Unable to initialize the rescaling context";
        return false;
    }
    ost->swsCtx = makeAVScopedPtr(swsCtx);

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

    const int linesize[1] = {getVideoFormatSize(frame->format.videoFormat) *
                             mFbWidth};

    // To test the speed of sws_scale()
    auto startUs = android::base::System::get()->getHighResTimeUs();
    auto data = frame->dataVec.data();
    sws_scale(ost->swsCtx.get(), (const uint8_t* const*)&data, linesize, 0,
              mFbHeight, ost->frame->data, ost->frame->linesize);
    D("Time to sws_scale: [%lld ms]\n",
      (long long)(android::base::System::get()->getHighResTimeUs() - startUs) /
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
            LOG(ERROR) << "Error while converting";
            return false;
        }
        mAudioStream.frame->pts = frame->pts;
        frame = mAudioStream.frame.get();
    }
    // NOTE: If frame is null, then this call is trying to flush out an audio
    // packet from the encoder.

    AVPacket pkt;
    int gotPacket;

    D("Encoding audio frame %d\n", mAudioStream.frameCount++);
    av_init_packet(&pkt);
    pkt.data = NULL;  // data and size must be 0
    pkt.size = 0;
    ret = avcodec_encode_audio2(c, &pkt, frame, &gotPacket);
    if (ret < 0) {
        LOG(ERROR) << "Error encoding audio frame: [" << avErr2Str(ret) << "]";
        return false;
    }

    if (gotPacket && pkt.size > 0) {
        if (!writeFrame(mAudioStream.stream, &pkt)) {
            LOG(ERROR) << "Error while writing audio frame: [" << avErr2Str(ret)
                       << "]";
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
        D("Encoding video frame %ld\n", mVideoStream.frameCount++);
        auto startUs = android::base::System::get()->getHighResTimeUs();
        ret = avcodec_encode_video2(c, &pkt, frame, &gotPacket);
        D("Time to avcodec_encode_video2: [%lld ms]\n",
          (long long)(android::base::System::get()->getHighResTimeUs() -
                      startUs) /
                  1000);
        if (ret < 0) {
            LOG(ERROR) << "Error encoding video frame: [" << avErr2Str(ret)
                       << "]";
            return false;
        }

        if (gotPacket && pkt.size > 0) {
            ret = writeFrame(mVideoStream.stream, &pkt) ? 0 : -1;
        } else {
            ret = 0;
        }
    }

    if (ret < 0) {
        LOG(ERROR) << "Error while writing video frame: [" << avErr2Str(ret)
                   << "]";
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
        LOG(ERROR) << "Error allocating an audio frame";
        return nullptr;
    }

    frame->format = sampleFmt;
    frame->channel_layout = channelLayout;
    frame->sample_rate = sampleRate;
    frame->nb_samples = nbSamples;

    if (nbSamples) {
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            LOG(ERROR) << "Error allocating an audio buffer";
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
        LOG(ERROR) << "Could not allocate video frame data.";
        return nullptr;
    }

    return picture;
}

bool FfmpegRecorderImpl::openAudioContext(AVCodec* codec,
                                          AVDictionary* optArgs) {

    return true;
}

void FfmpegRecorderImpl::closeAudioContext() {
    mAudioStream.frame.reset();
    mAudioStream.tmpFrame.reset();
    mAudioStream.codecCtx.reset();
    mAudioStream.swrCtx.reset();
}

void FfmpegRecorderImpl::closeVideoContext() {
    mVideoStream.frame.reset();
    mVideoStream.tmpFrame.reset();
    mVideoStream.codecCtx.reset();
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

void FfmpegRecorderImpl::logPacket(const AVFormatContext* fmtCtx,
                                   const AVPacket* pkt,
                                   AVMediaType type) {
    AVRational* time_base = &fmtCtx->streams[pkt->stream_index]->time_base;
    D("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s "
      "duration_time:%s "
      "stream_index:%d\n",
      avTs2Str(pkt->pts).c_str(), avTs2TimeStr(pkt->pts, time_base).c_str(),
      avTs2Str(pkt->dts).c_str(), avTs2TimeStr(pkt->dts, time_base).c_str(),
      avTs2Str(pkt->duration).c_str(),
      avTs2TimeStr(pkt->duration, time_base).c_str(), pkt->stream_index);
}
}  // namespace

// static
std::unique_ptr<FfmpegRecorder> FfmpegRecorder::create(
        uint16_t fbWidth,
        uint16_t fbHeight,
        android::base::StringView filename) {
    return std::unique_ptr<FfmpegRecorder>(
            new FfmpegRecorderImpl(fbWidth, fbHeight, filename));
}

}  // namespace recording
}  // namespace android
