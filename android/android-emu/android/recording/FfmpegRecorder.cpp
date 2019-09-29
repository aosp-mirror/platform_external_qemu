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

#include "android/base/Log.h"                   // for LogStream, LogMessage
#include "android/base/memory/ScopedPtr.h"      // for FuncDelete
#include "android/base/synchronization/Lock.h"  // for Lock, AutoLock
#include "android/base/system/System.h"         // for System
#include "android/recording/AVScopedPtr.h"      // for makeAVScopedPtr
#include "android/recording/Frame.h"            // for Frame, AVFormat, getV...
#include "android/recording/Producer.h"         // for Producer
#include "android/recording/codecs/Codec.h"     // for Codec, CodecParams
#include "android/utils/debug.h"                // for VERBOSE_record, VERBO...

extern "C" {
#include "libavformat/avformat.h"               // for AVStream, AVFormatCon...
#include "libavutil/avassert.h"                 // for av_assert0
#include "libavutil/mathematics.h"              // for av_rescale_rnd, AV_RO...
#include "libavutil/timestamp.h"                // for av_ts_make_string
#include "libswresample/swresample.h"           // for SwrContext, swr_alloc
#include "libswscale/swscale.h"                 // for sws_scale

namespace android {
namespace base {
class PathUtils;
}  // namespace base
}  // namespace android
struct SwsContext;
}

#include <assert.h>                             // for assert
#include <libavcodec/avcodec.h>                 // for AVCodecContext, AVPacket
#include <libavformat/avio.h>                   // for avio_open, AVIO_FLAG_...
#include <libavutil/avutil.h>                   // for AVMediaType, AVMEDIA_...
#include <libavutil/dict.h>                     // for AVDictionary
#include <libavutil/error.h>                    // for av_make_error_string
#include <libavutil/frame.h>                    // for AVFrame, av_frame_alloc
#include <libavutil/log.h>                      // for av_log_set_callback
#include <libavutil/pixfmt.h>                   // for AVPixelFormat, AV_PIX...
#include <libavutil/rational.h>                 // for AVRational
#include <libavutil/samplefmt.h>                // for AVSampleFormat
#include <stdarg.h>                             // for va_list
#include <stdio.h>                              // for vprintf, NULL
#include <string.h>                             // for memcpy
#include <cstdint>                              // for uint8_t
#include <functional>                           // for __base
#include <string>                               // for string, basic_string
#include <utility>                              // for move
#include <vector>                               // for vector

#define D(...) VERBOSE_PRINT(record, __VA_ARGS__)

namespace android {
namespace recording {

using android::base::AutoLock;
using android::base::Lock;
using android::base::PathUtils;

namespace {

// a wrapper around a single output AVStream
struct VideoOutputStream {
    // These two pointers are owned by the output context
    AVStream* stream = nullptr;
    AVScopedPtr<AVCodecContext> codecCtx;
    AVScopedPtr<AVFrame> frame;
    AVScopedPtr<AVFrame> tmpFrame;
    AVScopedPtr<SwsContext> swsCtx;

    uint64_t frameCount = 0;
    uint64_t writeFrameCount = 0;
};

struct AudioOutputStream {
    // These two pointers are owned by the output context
    AVStream* stream = nullptr;
    AVScopedPtr<AVCodecContext> codecCtx;
    AVScopedPtr<AVFrame> frame;
    AVScopedPtr<AVFrame> tmpFrame;
    AVScopedPtr<SwrContext> swrCtx;

    std::vector<uint8_t> audioLeftover;

    uint64_t frameCount = 0;
    uint64_t writeFrameCount = 0;
    // The timestamp of the next audio frame, expressed as an elapsed time difference
    // from |mStartTimeUs|.
    uint64_t next_tsUs = 0;
};

class FfmpegRecorderImpl : public FfmpegRecorder {
public:
    // Ctor
    FfmpegRecorderImpl(uint16_t fbWidth,
                       uint16_t fbHeight,
                       android::base::StringView filename,
                       android::base::StringView containerFormat);

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
    bool initOutputContext(android::base::StringView filename,
                           android::base::StringView containerFormat);

    void attachAudioProducer(std::unique_ptr<Producer> producer);
    void attachVideoProducer(std::unique_ptr<Producer> producer);

    // Callbacks to pass to the producers to encode the new av frames.
    bool encodeAudioFrame(const Frame* audioFrame);
    bool encodeVideoFrame(const Frame* videoFrame);

    // Interleave the packets
    bool writeFrame(const AVCodecContext* c, AVStream* stream, AVPacket* pkt);

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

    // Ordered by verbosity
    enum class AvLogLevel {
        Quiet = AV_LOG_QUIET,
        Panic = AV_LOG_PANIC,
        Fatal = AV_LOG_FATAL,
        Error = AV_LOG_ERROR,
        Warning = AV_LOG_WARNING,
        Info = AV_LOG_INFO,
        Verbose = AV_LOG_VERBOSE,
        Debug = AV_LOG_DEBUG,
        Trace = AV_LOG_TRACE,
    };
    // Enable ffmpeg logging. Useful for tracking down bugs in ffmpeg.
    static void enableFfmpegLogging(AvLogLevel level);
    // Callback for av_log_set_callback() to get logging info
    static void avLoggingCallback(void *ptr, int level, const char *fmt, va_list vargs);

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

FfmpegRecorderImpl::FfmpegRecorderImpl(
        uint16_t fbWidth,
        uint16_t fbHeight,
        android::base::StringView filename,
        android::base::StringView containerFormat)
    : mFbWidth(fbWidth), mFbHeight(fbHeight) {
    assert(mFbWidth > 0 && mFbHeight > 0);
    mValid = initOutputContext(filename, containerFormat);

    enableFfmpegLogging(AvLogLevel::Trace);
}

FfmpegRecorderImpl::~FfmpegRecorderImpl() {
    abortRecording();
}

bool FfmpegRecorderImpl::isValid() {
    return mValid;
}

bool FfmpegRecorderImpl::initOutputContext(
        android::base::StringView filename,
        android::base::StringView containerFormat) {
    if (mStarted) {
        LOG(ERROR) << ": Recording already started";
        return false;
    }
    if (filename.empty() || containerFormat.empty()) {
        LOG(ERROR) << __func__
                   << "No output filename or container format supplied";
        return false;
    }

    mEncodedOutputPath = filename;

    // Initialize libavcodec, and register all codecs and formats. does not hurt
    // to register multiple times
    av_register_all();

    // allocate the output media context
    AVFormatContext* outputCtx;
    avformat_alloc_output_context2(&outputCtx, nullptr,
                                   android::base::c_str(containerFormat),
                                   mEncodedOutputPath.c_str());
    if (outputCtx == nullptr) {
        LOG(ERROR) << "avformat_alloc_output_context2 failed";
        return false;
    }
    mOutputContext = makeAVScopedPtr(outputCtx);

    AVOutputFormat* fmt = mOutputContext->oformat;
    // open the output file, if needed
    if (!(fmt->flags & AVFMT_NOFILE)) {
        int ret = avio_open(&mOutputContext->pb, mEncodedOutputPath.c_str(),
                            AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOG(ERROR) << "Could not open [" << mEncodedOutputPath
                       << "]: " << avErr2Str(ret);
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

    if (mAudioProducer) {
        mAudioProducer->stop();
        mAudioProducer->wait();
    }
    mVideoProducer->stop();
    mVideoProducer->wait();

    mValid = false;
}

bool FfmpegRecorderImpl::stop() {
    assert(mValid);

    if (!mStarted) {
        return false;
    }
    mStarted = false;

    if (mAudioProducer) {
        mAudioProducer->stop();
        mAudioProducer->wait();
    }
    mVideoProducer->stop();
    mVideoProducer->wait();

    // flush video encoding with a NULL frame
    if (mHasVideoTrack) {
        while (true) {
            AVPacket pkt = {0};
            int gotPacket = 0;
            av_init_packet(&pkt);
            int ret = avcodec_encode_video2(mVideoStream.codecCtx.get(), &pkt,
                                            nullptr, &gotPacket);
            if (ret < 0 || !gotPacket) {
                break;
            }
            VLOG(record) << "Writing video frame "
                         << mVideoStream.writeFrameCount++;
            writeFrame(mVideoStream.codecCtx.get(), mVideoStream.stream, &pkt);
        }
    }

    // flush the remaining audio packet
    if (mHasAudioTrack) {
        if (mAudioStream.audioLeftover.size() > 0) {
            auto frameSize = mAudioStream.audioLeftover.capacity();
            if (mAudioStream.audioLeftover.size() <
                frameSize) {  // this should always true
                auto size = frameSize - mAudioStream.audioLeftover.size();
                Frame f(size, 0);
                // compute the time delta between frames
                auto avframe = mAudioStream.frame.get();
                uint64_t deltaUs = (uint64_t)(((float)avframe->nb_samples / avframe->sample_rate) * 1000000);
                f.tsUs = mAudioStream.next_tsUs;
                mAudioStream.next_tsUs += deltaUs;
                f.format.audioFormat = mAudioProducer->getFormat().audioFormat;
                encodeAudioFrame(&f);
            }
        }

        // flush audio encoding with a NULL frame
        while (true) {
            AVPacket pkt = {0};
            int gotPacket;
            av_init_packet(&pkt);
            int ret = avcodec_encode_audio2(mAudioStream.codecCtx.get(), &pkt,
                                            NULL, &gotPacket);
            if (ret < 0 || !gotPacket) {
                break;
            }
            VLOG(record) << "Writing audio frame "
                         << mAudioStream.writeFrameCount++;
            writeFrame(mAudioStream.codecCtx.get(), mAudioStream.stream, &pkt);
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
    assert(mValid && codec != nullptr && producer != nullptr);
    if (mStarted) {
        LOG(ERROR) << "Muxer already started";
        return false;
    }
    if (mHasAudioTrack) {
        LOG(ERROR) << "An audio track was already added";
        return false;
    }

    if (codec->getCodec()->type != AVMEDIA_TYPE_AUDIO) {
        LOG(ERROR) << avcodec_get_name(codec->getCodecId())
                   << " is not an audio codec.";
        return false;
    }

    auto params = codec->getCodecParams();
    VLOG(record) << "bitrate=[" << params->bitrate << "], sample_rate=["
                 << params->sample_rate << "])";

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

    // allocate an AVCodecContext and set its fields to default values.
    // avcodec_alloc_context3() will allocate private data and initialize
    // defaults for the given codec, so make sure that:
    //   1) avcodec_open2() is not called with a different codec,
    //   2) the AVCodecContext is freed with avcodec_free_context().
    AVCodecContext* c = avcodec_alloc_context3(codec->getCodec());
    if (c == nullptr) {
        LOG(ERROR) << "avcodec_alloc_context3 failed [codec="
                   << avcodec_get_name(codec->getCodecId()) << "]";
        return false;
    }
    ost->codecCtx = makeAVScopedPtr(c);

    if (!codec->configAndOpenEncoder(oc, c, ost->stream)) {
        LOG(ERROR) << "Unable to open video codec context ["
                   << avcodec_get_name(codec->getCodecId()) << "]";
        return false;
    }

    /* copy the stream parameters to the muxer */
    if (avcodec_parameters_from_context(ost->stream->codecpar, c) < 0) {
        LOG(ERROR) << "Could not copy the stream parameters";
        return false;
    }

    VLOG(record) << "c->sample_fmt=" << c->sample_fmt
                 << ", c->channels=" << c->channels
                 << ", ost->st->time_base.den=" << ost->stream->time_base.den;

    uint32_t nbSamples;
    if (codec->getCodec()->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE) {
        nbSamples = 10000;
    } else {
        VLOG(record) << "setting nbSamples=" << c->frame_size;
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
            ost->frame->nb_samples * ost->codecCtx->channels *
            getAudioFormatSize(mAudioProducer->getFormat().audioFormat);
    ost->audioLeftover.reserve(frameSize);

    mHasAudioTrack = true;
    return true;
}

bool FfmpegRecorderImpl::addVideoTrack(
        std::unique_ptr<Producer> producer,
        const Codec<SwsContext*>* codec) {
    assert(mValid && codec != nullptr && producer != nullptr);
    if (mStarted) {
        LOG(ERROR) << "Muxer already started";
        return false;
    }
    if (mHasVideoTrack) {
        LOG(ERROR) << "A video track was already added";
        return false;
    }

    if (codec->getCodec()->type != AVMEDIA_TYPE_VIDEO) {
        LOG(ERROR) << avcodec_get_name(codec->getCodecId())
                   << " is not a video codec.";
        return false;
    }

    attachVideoProducer(std::move(producer));

    VideoOutputStream* ost = &mVideoStream;
    AVFormatContext* oc = mOutputContext.get();

    auto codecParams = codec->getCodecParams();
    VLOG(record) << "width=[" << codecParams->width << "], height=["
                 << codecParams->height << "], bitRate=["
                 << codecParams->bitrate << "], fps=[" << codecParams->fps
                 << "], intraSpacing=[" << codecParams->intra_spacing << "])";

    ost->frameCount = 0;
    ost->writeFrameCount = 0;

    auto stream = avformat_new_stream(oc, codec->getCodec());
    if (!stream) {
        LOG(ERROR) << "Could not allocate video stream";
        return false;
    }
    ost->stream = stream;

    // allocate an AVCodecContext and set its fields to default values.
    // avcodec_alloc_context3() will allocate private data and initialize
    // defaults for the given codec, so make sure that:
    //   1) avcodec_open2() is not called with a different codec,
    //   2) the AVCodecContext is freed with avcodec_free_context().
    AVCodecContext* c = avcodec_alloc_context3(codec->getCodec());
    if (c == nullptr) {
        LOG(ERROR) << ": avcodec_alloc_context3 failed [codec="
                   << avcodec_get_name(codec->getCodecId()) << "]";
        return false;
    }
    ost->codecCtx = makeAVScopedPtr(c);

    if (!codec->configAndOpenEncoder(oc, c, ost->stream)) {
        LOG(ERROR) << "Unable to open video codec context ["
                   << avcodec_get_name(codec->getCodecId()) << "]";
        return false;
    }

    /* copy the stream parameters to the muxer */
    if (avcodec_parameters_from_context(ost->stream->codecpar, c) < 0) {
        LOG(ERROR) << "Could not copy the stream parameters";
        return false;
    }

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

    ost->next_tsUs = frame->tsUs - mStartTimeUs;
    AVFrame* avframe = ost->tmpFrame.get();
    auto frameSize = ost->audioLeftover.capacity();

    // we need to split into frames
    auto remaining = frame->dataVec.size();
    auto buf = static_cast<const uint8_t*>(frame->dataVec.data());
    // frame_size is also number of samples. frameDurationUs is the duration, in microseconds,
    // for a frame in this codec configuration.
    uint64_t frameDurationUs = (uint64_t)((double)ost->codecCtx->frame_size / ost->codecCtx->sample_rate * 1000000);

    // If we have leftover data, fill the rest of the frame and send to the
    // encoder.
    if (ost->audioLeftover.size() > 0) {
        if (ost->audioLeftover.size() + remaining >= frameSize) {
            auto bufUsed = frameSize - ost->audioLeftover.size();
            ost->audioLeftover.insert(ost->audioLeftover.end(), buf,
                                      buf + bufUsed);
            memcpy(avframe->data[0], ost->audioLeftover.data(), frameSize);
            // We need to guesstimate the timestamp from the last encoded timestamp, because the frame
            // size required by the codec may be smaller than the incoming frame size. In the case of
            // using vorbis with the matroska muxer in ffmpeg 3.4.5, the codec wants 64 samples per frame,
            // but |frame| is giving us 512 samples. We are making an assumption here that the audio
            // coming in is given to us at a constant rate. It we have any moments of no data, we may
            // experience some audio lag.
            avframe->pts = (int64_t)ost->next_tsUs;
            ost->next_tsUs += frameDurationUs;
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
        // We need to guesstimate the timestamp from the last encode timestamp, because the frame
        // size required by the codec may be smaller than the incoming frame size. We'll Assume that the audio
        // starts at time=0ms.
        avframe->pts = (int64_t)ost->next_tsUs;
        ost->next_tsUs += frameDurationUs;
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
    VLOG(record)
            << "Time to sws_scale: ["
            << (long long)(android::base::System::get()->getHighResTimeUs() -
                           startUs) /
                       1000
            << " ms]";

    uint64_t elapsedUS = frame->tsUs - mStartTimeUs;
    ost->frame->pts = (int64_t)(elapsedUS);

    bool ret = writeVideoFrame(ost->frame.get());
    mHasVideoFrames = true;

    return ret;
}

bool FfmpegRecorderImpl::writeFrame(const AVCodecContext* c, AVStream* stream, AVPacket* pkt) {
    pkt->stream_index = stream->index;

    // Use the container's time_base. For the screen recorder, the codec's time base
    // should be set to a millisecond timebase, because the pts we set on the frame is a number
    // of milliseconds.
    av_packet_rescale_ts(pkt, c->time_base, stream->time_base);
    logPacket(mOutputContext.get(), pkt, c->codec->type);

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
    AVCodecContext* c = mAudioStream.codecCtx.get();

    if (frame) {
        // convert samples from native format to destination codec format, using
        // the resampler compute destination number of samples
        dstNbSamples = av_rescale_rnd(
                swr_get_delay(mAudioStream.swrCtx.get(), c->sample_rate) +
                        frame->nb_samples,
                c->sample_rate, c->sample_rate, AV_ROUND_UP);
        VLOG(record) << "dstNbSamples=[" << dstNbSamples
                     << "], frame->nb_samples=[" << frame->nb_samples << "]";
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

    VLOG(record) << "Encoding audio frame " << mAudioStream.frameCount++;
    av_init_packet(&pkt);
    pkt.data = NULL;  // data and size must be 0
    pkt.size = 0;
    ret = avcodec_encode_audio2(c, &pkt, frame, &gotPacket);
    if (ret < 0) {
        LOG(ERROR) << "Error encoding audio frame: [" << avErr2Str(ret) << "]";
        return false;
    }

    if (gotPacket && pkt.size > 0) {
        if (!writeFrame(mAudioStream.codecCtx.get(), mAudioStream.stream, &pkt)) {
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

    c = mVideoStream.codecCtx.get();

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
        VLOG(record) << "Encoding video frame " << mVideoStream.frameCount++;
        auto startUs = android::base::System::get()->getHighResTimeUs();
        ret = avcodec_encode_video2(c, &pkt, frame, &gotPacket);
        VLOG(record) << "Time to avcodec_encode_video2: ["
                     << (long long)(android::base::System::get()
                                            ->getHighResTimeUs() -
                                    startUs) /
                                1000
                     << " ms]";

        if (ret < 0) {
            LOG(ERROR) << "Error encoding video frame: [" << avErr2Str(ret)
                       << "]";
            return false;
        }

        if (gotPacket && pkt.size > 0) {
            ret = writeFrame(mVideoStream.codecCtx.get(), mVideoStream.stream, &pkt) ? 0 : -1;
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
        LOG(ERROR) << "Could not allocate video frame data ("
                   << "pixFmt=" << pixFmt
                   << ",w=" << width
                   << ",h=" << height << ")";
        return nullptr;
    }

    return picture;
}

bool FfmpegRecorderImpl::openAudioContext(AVCodec* codec,
                                          AVDictionary* optArgs) {

    return true;
}

void FfmpegRecorderImpl::closeAudioContext() {
    mAudioStream.codecCtx.reset();
    mAudioStream.frame.reset();
    mAudioStream.tmpFrame.reset();
    mAudioStream.swrCtx.reset();
}

void FfmpegRecorderImpl::closeVideoContext() {
    mVideoStream.codecCtx.reset();
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

void FfmpegRecorderImpl::logPacket(const AVFormatContext* fmtCtx,
                                   const AVPacket* pkt,
                                   AVMediaType type) {
    AVRational* time_base = &fmtCtx->streams[pkt->stream_index]->time_base;
    VLOG(record) << "pts:" << avTs2Str(pkt->pts).c_str()
                 << " pts_time:" << avTs2TimeStr(pkt->pts, time_base).c_str()
                 << " dts:" << avTs2Str(pkt->dts).c_str()
                 << " dts_time:" << avTs2TimeStr(pkt->dts, time_base).c_str()
                 << " duration:" << avTs2Str(pkt->duration).c_str()
                 << " duration_time:"
                 << avTs2TimeStr(pkt->duration, time_base).c_str()
                 << " stream_index:" << pkt->stream_index;
}

// static
void FfmpegRecorderImpl::enableFfmpegLogging(AvLogLevel level) {
    av_log_set_level(static_cast<int>(level));
    av_log_set_callback(avLoggingCallback);
}

// static
void FfmpegRecorderImpl::avLoggingCallback(void* ptr, int level, const char* fmt, va_list vargs) {
    if (VERBOSE_CHECK(record)) {
        // It seems ffmpeg doesn't silence repeated warnings. So let's just
        // capture it here and ignore it if we aren't debugging it.
        vprintf(fmt, vargs);
    }
}
}  // namespace

// static
std::unique_ptr<FfmpegRecorder> FfmpegRecorder::create(
        uint16_t fbWidth,
        uint16_t fbHeight,
        android::base::StringView filename,
        android::base::StringView containerFormat) {
    return std::unique_ptr<FfmpegRecorder>(new FfmpegRecorderImpl(
            fbWidth, fbHeight, filename, containerFormat));
}

}  // namespace recording
}  // namespace android
