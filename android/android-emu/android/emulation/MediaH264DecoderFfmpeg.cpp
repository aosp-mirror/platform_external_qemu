// Copyright (C) 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/emulation/MediaH264DecoderFfmpeg.h"
#include "android/base/system/System.h"
#include "android/emulation/H264NaluParser.h"
#include "android/emulation/H264PingInfoParser.h"
#include "android/emulation/YuvConverter.h"

#include <cstdint>
#include <string>
#include <vector>

#include <stdio.h>
#include <string.h>

#define MEDIA_H264_DEBUG 0

#if MEDIA_H264_DEBUG
#define H264_DPRINT(fmt,...) fprintf(stderr, "h264-ffmpeg-dec: %s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define H264_DPRINT(fmt,...)
#endif

namespace android {
namespace emulation {

using InitContextParam = H264PingInfoParser::InitContextParam;
using DecodeFrameParam = H264PingInfoParser::DecodeFrameParam;
using ResetParam = H264PingInfoParser::ResetParam;
using GetImageParam = H264PingInfoParser::GetImageParam;
MediaH264DecoderFfmpeg::~MediaH264DecoderFfmpeg() {
    H264_DPRINT("destroyed MediaH264DecoderFfmpeg %p", this);
    destroyH264Context();
}

void MediaH264DecoderFfmpeg::reset(void* ptr) {
    destroyH264Context();
    ResetParam param{};
    mParser.parseResetParams(ptr, param);
    initH264ContextInternal(param.width, param.height, param.outputWidth,
                            param.outputHeight, param.outputPixelFormat);
}

void MediaH264DecoderFfmpeg::initH264Context(void* ptr) {
    InitContextParam param{};
    mParser.parseInitContextParams(ptr, param);
    initH264ContextInternal(param.width, param.height, param.outputWidth,
                            param.outputHeight, param.outputPixelFormat);
}

void MediaH264DecoderFfmpeg::initH264ContextInternal(unsigned int width,
                                                     unsigned int height,
                                                     unsigned int outWidth,
                                                     unsigned int outHeight,
                                                     PixelFormat outPixFmt) {
    H264_DPRINT("%s(w=%u h=%u out_w=%u out_h=%u pixfmt=%u)",
                __func__, width, height, outWidth, outHeight, (uint8_t)outPixFmt);
    mWidth = width;
    mHeight = height;
    mOutputWidth = outWidth;
    mOutputHeight = outHeight;
    mOutPixFmt = outPixFmt;
    mOutBufferSize = outWidth * outHeight * 3 / 2;

    mIsInFlush = false;

    mDecodedFrame.resize(mOutBufferSize);

    // standard ffmpeg codec stuff
    avcodec_register_all();
    if(0){
        AVCodec* current_codec = NULL;

        current_codec = av_codec_next(current_codec);
        while (current_codec != NULL)
        {
            if (av_codec_is_decoder(current_codec))
            {
                H264_DPRINT("codec decoder found %s long name %s", current_codec->name, current_codec->long_name);
            }
            current_codec = av_codec_next(current_codec);
        }
    }

    mCodec = NULL;
    auto useCuvidEnv = android::base::System::getEnvironmentVariable(
            "ANDROID_EMU_CODEC_USE_FFMPEG_CUVID_DECODER");
    if (useCuvidEnv != "") {
        mCodec = avcodec_find_decoder_by_name("h264_cuvid");
        if (mCodec) {
            mIsSoftwareDecoder = false;
            H264_DPRINT("Found h264_cuvid decoder, using it");
        } else {
            H264_DPRINT("Cannot find h264_cuvid decoder");
        }
    }
    if (!mCodec) {
        mCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
        H264_DPRINT("Using default software h264 decoder");
    }
    mCodecCtx = avcodec_alloc_context3(mCodec);

    mCodecCtx->thread_count = 4;
    mCodecCtx->thread_type = FF_THREAD_FRAME;
    avcodec_open2(mCodecCtx, mCodec, 0);
    mFrame = av_frame_alloc();

    H264_DPRINT("Successfully created software h264 decoder context %p", mCodecCtx);
}

MediaH264DecoderPlugin* MediaH264DecoderFfmpeg::clone() {
    H264_DPRINT("clone MediaH264DecoderFfmpeg %p with version %d", this,
                (int)mParser.version());
    return new MediaH264DecoderFfmpeg(mId, mParser);
}

MediaH264DecoderFfmpeg::MediaH264DecoderFfmpeg(uint64_t id,
                                               H264PingInfoParser parser)
    : mId(id), mParser(parser) {
    H264_DPRINT("allocated MediaH264DecoderFfmpeg %p with version %d", this,
                (int)mParser.version());
}
void MediaH264DecoderFfmpeg::destroyH264Context() {
    H264_DPRINT("Destroy context %p", this);
    if (mCodecCtx) {
        avcodec_close(mCodecCtx);
        av_free(mCodecCtx);
        mCodecCtx = NULL;
    }
    if (mFrame) {
        av_frame_free(&mFrame);
        mFrame = NULL;
    }
}

void MediaH264DecoderFfmpeg::resetDecoder() {
    mNumDecodedFrame = 0;
    avcodec_close(mCodecCtx);
    av_free(mCodecCtx);
    mCodecCtx = avcodec_alloc_context3(mCodec);
    avcodec_open2(mCodecCtx, mCodec, 0);
}

bool MediaH264DecoderFfmpeg::checkWhetherConfigChanged(const uint8_t* frame, size_t szBytes) {
    // get frame type
    // if the frame is none SPS/PPS, return false
    // otherwise, check both SPS/PPS and return true
    const uint8_t* currNalu = H264NaluParser::getNextStartCodeHeader(frame, szBytes);
    if (currNalu == nullptr) {
        // should not happen
        H264_DPRINT("Found bad frame");
        return false;
    }

    size_t remaining = szBytes - (currNalu - frame);
    size_t currNaluSize = remaining;
    H264NaluParser::H264NaluType currNaluType = H264NaluParser::getFrameNaluType(currNalu, currNaluSize, NULL);
    if (currNaluType != H264NaluParser::H264NaluType::SPS) {
        return false;
    }

    H264_DPRINT("found SPS\n");

    const uint8_t* nextNalu = H264NaluParser::getNextStartCodeHeader(currNalu + 3, remaining - 3);

    if (nextNalu == nullptr) {
        // only one nalu, cannot have configuration change
        H264_DPRINT("frame has only one Nalu unit, cannot be configuration change\n");
        return false;
    }

    if (mNumDecodedFrame == 0) {
        H264_DPRINT("have not decoded anything yet, cannot be config change");
        return false;
    }
    // pretty sure it is config change
    H264_DPRINT("\n\nDetected stream configuration change !!!\n\n");
    return true;
}

void MediaH264DecoderFfmpeg::decodeFrameDirect(void* ptr,
                                               const uint8_t* frame,
                                               size_t szBytes,
                                               uint64_t pts) {
    DecodeFrameParam param{};
    mParser.parseDecodeFrameParams(ptr, param);
    param.pData = (uint8_t*)frame;
    param.pts = pts;
    param.size = szBytes;
    decodeFrameInternal(param);
}

void MediaH264DecoderFfmpeg::decodeFrame(void* ptr) {
    DecodeFrameParam param{};
    mParser.parseDecodeFrameParams(ptr, param);
    decodeFrameInternal(param);
}

void MediaH264DecoderFfmpeg::decodeFrameInternal(DecodeFrameParam& param) {
    const uint8_t* frame = param.pData;
    size_t szBytes = param.size;
    uint64_t inputPts = param.pts;

    H264_DPRINT("%s(frame=%p, sz=%zu pts %lld)", __func__, frame, szBytes, (long long)inputPts);
    Err h264Err = Err::NoErr;
    // TODO: move this somewhere else
    // First return parameter is the number of bytes processed,
    // Second return parameter is the error code
    size_t* retSzBytes = param.pConsumedBytes;
    int32_t* retErr = param.pDecoderErrorCode;

    const bool enableSnapshot = true;
    if (enableSnapshot) {
        std::vector<uint8_t> v;
        v.assign(frame, frame + szBytes);
        bool hasSps = H264NaluParser::checkSpsFrame(frame, szBytes);
        if (hasSps) {
            mSnapshotState = SnapshotState{};
            mSnapshotState.saveSps(v);
        } else {
            bool hasPps = H264NaluParser::checkPpsFrame(frame, szBytes);
            if (hasPps) {
                mSnapshotState.savePps(v);
                mSnapshotState.savedPackets.clear();
                mSnapshotState.savedDecodedFrame.data.clear();
            } else {
                bool isIFrame = H264NaluParser::checkIFrame(frame, szBytes);
                if (isIFrame) {
                    mSnapshotState.savedPackets.clear();
                }
                const bool saveOK = mSnapshotState.savePacket(std::move(v), inputPts);
                if (saveOK) {
                    H264_DPRINT("saving packet; total is %d",
                            (int)(mSnapshotState.savedPackets.size()));
                } else {
                    H264_DPRINT("saving packet; has duplicate, skip; total is %d",
                            (int)(mSnapshotState.savedPackets.size()));
                    *retSzBytes = szBytes;
                    *retErr = (int32_t)h264Err;
                    mImageReady = true;
                    return;
                }
            }
        }
    }

    if (!mIsSoftwareDecoder) {
        bool configChanged = checkWhetherConfigChanged(frame, szBytes);
        if (configChanged) {
            resetDecoder();
        }
    }
    av_init_packet(&mPacket);
    mPacket.data = (unsigned char*)frame;
    mPacket.size = szBytes;
    mPacket.pts = inputPts;
    avcodec_send_packet(mCodecCtx, &mPacket);
    int retframe = avcodec_receive_frame(mCodecCtx, mFrame);
    *retSzBytes = szBytes;
    *retErr = (int32_t)h264Err;
    mIsInFlush = false;
    if (retframe != 0) {
        H264_DPRINT("decodeFrame has nonzero return value %d", retframe);
        if (retframe == AVERROR_EOF) {
            H264_DPRINT("EOF returned from decoder");
            H264_DPRINT("EOF returned from decoder reset context now");
            resetDecoder();
        } else if (retframe == AVERROR(EAGAIN)) {
            H264_DPRINT("EAGAIN returned from decoder");
        } else {
            H264_DPRINT("unknown value %d", retframe);
        }
        return;
    }
    H264_DPRINT("new w %d new h %d, old w %d old h %d",
                mFrame->width, mFrame->height,
                mOutputWidth, mOutputHeight);
    mFrameFormatChanged = false;
    ++mNumDecodedFrame;
    copyFrame();
    H264_DPRINT("%s: got frame in decode mode", __func__);
    mImageReady = true;
}

void MediaH264DecoderFfmpeg::copyFrame() {
    int w = mFrame->width;
    int h = mFrame->height;
    if (w != mOutputWidth || h != mOutputHeight) {
        mOutputWidth = w;
        mOutputHeight= h;
        mOutBufferSize = mOutputWidth * mOutputHeight * 3 / 2;
        mDecodedFrame.resize(mOutBufferSize);
    }
    H264_DPRINT("w %d h %d Y line size %d U line size %d V line size %d", w, h,
            mFrame->linesize[0], mFrame->linesize[1], mFrame->linesize[2]);
    for (int i = 0; i < h; ++i) {
        memcpy(mDecodedFrame.data() + i * w,
               mFrame->data[0] + i * mFrame->linesize[0], w);
    }
    H264_DPRINT("format is %d and NV21 is %d  12 is %d", mFrame->format, (int)AV_PIX_FMT_NV21,
            (int)AV_PIX_FMT_NV12);
    if (mFrame->format == AV_PIX_FMT_NV12) {
        for (int i=0; i < h / 2; ++i) {
            memcpy(w * h + mDecodedFrame.data() + i * w,
                   mFrame->data[1] + i * mFrame->linesize[1], w);
        }
        YuvConverter<uint8_t> convert8(mOutputWidth, mOutputHeight);
        convert8.UVInterleavedToPlanar(mDecodedFrame.data());
    } else {
        for (int i=0; i < h / 2; ++i) {
            memcpy(w * h + mDecodedFrame.data() + i * w / 2,
                   mFrame->data[1] + i * mFrame->linesize[1], w / 2);
        }
        for (int i=0; i < h / 2; ++i) {
            memcpy(w * h + w * h / 4 + mDecodedFrame.data() + i * w / 2,
                   mFrame->data[2] + i * mFrame->linesize[2], w / 2);
        }
    }
    mColorPrimaries = mFrame->color_primaries;
    mColorRange = mFrame->color_range;
    mColorTransfer = mFrame->color_trc;
    mColorSpace = mFrame->colorspace;
    mOutputPts = mFrame->pts;
    H264_DPRINT("copied Frame and it has presentation time at %lld", (long long)(mFrame->pts));
    H264_DPRINT("Frame primary %d range %d transfer %d space %d", mFrame->color_primaries,
            mFrame->color_range, mFrame->color_trc, mFrame->colorspace);
}

void MediaH264DecoderFfmpeg::flush(void* ptr) {
    H264_DPRINT("Flushing...");
    mIsInFlush = true;
    mSnapshotState = SnapshotState{};
    H264_DPRINT("Flushing done");
}

void MediaH264DecoderFfmpeg::getImage(void* ptr) {
    H264_DPRINT("getImage %p", ptr);
    GetImageParam param{};
    mParser.parseGetImageParams(ptr, param);

    int* retErr = param.pDecoderErrorCode;
    uint32_t* retWidth = param.pRetWidth;
    uint32_t* retHeight = param.pRetHeight;
    uint64_t* retPts = param.pRetPts;
    uint32_t* retColorPrimaries = param.pRetColorPrimaries;
    uint32_t* retColorRange = param.pRetColorRange;
    uint32_t* retColorTransfer = param.pRetColorTransfer;
    uint32_t* retColorSpace = param.pRetColorSpace;

    static int numbers=0;
    //H264_DPRINT("calling getImage %d", numbers++);
    if (!mImageReady) {
        if (mFrameFormatChanged) {
            *retWidth = mOutputWidth;
            *retHeight = mOutputHeight;
            *retErr = static_cast<int>(Err::DecoderRestarted);
            return;
        }
        if (mIsInFlush) {
            // guest be in flush mode, so try to get a frame
            avcodec_send_packet(mCodecCtx, NULL);
            int retframe = avcodec_receive_frame(mCodecCtx, mFrame);
            if (retframe == AVERROR(EAGAIN) || retframe == AVERROR_EOF) {
                H264_DPRINT("%s: frame is null", __func__);
                *retErr = static_cast<int>(Err::NoDecodedFrame);
                return;
            }

            if (retframe != 0) {
                char tmp[1024];
                av_strerror(retframe, tmp, sizeof(tmp));
                H264_DPRINT("WARNING: some unknown error %d: %s", retframe,
                            tmp);
                *retErr = static_cast<int>(Err::NoDecodedFrame);
                return;
            }
            H264_DPRINT("%s: got frame in flush mode retrun code %d", __func__, retframe);
            //now copy to mDecodedFrame
            copyFrame();
            mImageReady = true;
        } else {
            H264_DPRINT("%s: no new frame yet", __func__);
            *retErr = static_cast<int>(Err::NoDecodedFrame);
            return;
        }
    }

    *retWidth = mOutputWidth;
    *retHeight = mOutputHeight;
    *retPts = mOutputPts;
    *retColorPrimaries = mColorPrimaries;
    *retColorRange = mColorRange;
    *retColorTransfer = mColorTransfer;
    *retColorSpace = mColorSpace;

    if (mParser.version() == 100) {
        uint8_t* dst = param.pDecodedFrame;
        memcpy(dst, mDecodedFrame.data(), mOutBufferSize);
    } else if (mParser.version() == 200) {
        if (param.hostColorBufferId >= 0) {
            mRenderer.renderToHostColorBuffer(param.hostColorBufferId,
                                              mOutputWidth, mOutputHeight,
                                              mDecodedFrame.data());
        } else {
            uint8_t* dst = param.pDecodedFrame;
            memcpy(dst, mDecodedFrame.data(), mOutBufferSize);
        }
    }

    mImageReady = false;
    *retErr = mOutBufferSize;
    H264_DPRINT("getImage %p done", ptr);
}

void MediaH264DecoderFfmpeg::save(base::Stream* stream) const {
    stream->putBe32(mParser.version());
    stream->putBe32(mWidth);
    stream->putBe32(mHeight);
    stream->putBe32((int)mOutPixFmt);

    const int hasContext = mCodecCtx != nullptr ? 1 : 0;
    stream->putBe32(hasContext);

    if (mImageReady) {
        mSnapshotState.saveDecodedFrame(
                mDecodedFrame, mOutputWidth, mOutputHeight,
                ColorAspects{mColorPrimaries, mColorRange, mColorTransfer,
                             mColorSpace},
                mOutputPts);
    } else {
        mSnapshotState.savedDecodedFrame.data.clear();
    }
    H264_DPRINT("saving packets now %d",
                (int)(mSnapshotState.savedPackets.size()));
    mSnapshotState.save(stream);
}

void MediaH264DecoderFfmpeg::oneShotDecode(std::vector<uint8_t>& data,
                                           uint64_t pts) {
    av_init_packet(&mPacket);
    mPacket.data = (unsigned char*)(data.data());
    mPacket.size = data.size();
    mPacket.pts = pts;
    H264_DPRINT("decoding pts %lld packet size %d", (long long)pts, (int)data.size());
    avcodec_send_packet(mCodecCtx, &mPacket);
    avcodec_receive_frame(mCodecCtx, mFrame);
}

bool MediaH264DecoderFfmpeg::load(base::Stream* stream) {
    uint32_t version = stream->getBe32();
    mParser = H264PingInfoParser{version};

    mWidth = stream->getBe32();
    mHeight = stream->getBe32();
    mOutPixFmt = (PixelFormat)stream->getBe32();

    const int hasContext = stream->getBe32();
    if (hasContext) {
        initH264ContextInternal(mWidth, mHeight, mWidth, mHeight, mOutPixFmt);
        assert(mCodecCtx != nullptr);
    }

    mSnapshotState.load(stream);

    H264_DPRINT("loaded packets %d, now restore decoder",
                (int)(mSnapshotState.savedPackets.size()));
    if (hasContext && mCodecCtx != nullptr && mSnapshotState.sps.size() > 0) {
        oneShotDecode(mSnapshotState.sps, 0);
        if (mSnapshotState.pps.size() > 0) {
            oneShotDecode(mSnapshotState.pps, 0);
            if (mSnapshotState.savedPackets.size() > 0) {
                for (int i = 0; i < mSnapshotState.savedPackets.size(); ++i) {
                    PacketInfo& pkt = mSnapshotState.savedPackets[i];
                    oneShotDecode(pkt.data, pkt.pts);
                }
                copyFrame(); //save the last frame
            }
        }
    }

    if (mSnapshotState.savedDecodedFrame.data.size() > 0) {
        mDecodedFrame = mSnapshotState.savedDecodedFrame.data;
        mOutBufferSize = mSnapshotState.savedDecodedFrame.data.size();
        mOutputWidth = mSnapshotState.savedDecodedFrame.width;
        mOutputHeight = mSnapshotState.savedDecodedFrame.height;
        mColorPrimaries = mSnapshotState.savedDecodedFrame.color.primaries;
        mColorRange = mSnapshotState.savedDecodedFrame.color.range;
        mColorTransfer = mSnapshotState.savedDecodedFrame.color.transfer;
        mColorSpace = mSnapshotState.savedDecodedFrame.color.space;
        mOutputPts = mSnapshotState.savedDecodedFrame.pts;
        mImageReady = true;
    } else {
        mImageReady = false;
    }
    H264_DPRINT("Done loading snapshots frames\n\n");
    return true;
}

}  // namespace emulation
}  // namespace android

