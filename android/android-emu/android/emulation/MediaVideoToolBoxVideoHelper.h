// Copyright (C) 2021 The Android Open Source Project
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

#pragma once

#include "host-common/H264NaluParser.h"
#include "host-common/MediaFfmpegVideoHelper.h"
#include "host-common/MediaSnapshotState.h"
#include "host-common/MediaTexturePool.h"
#include "host-common/MediaVideoHelper.h"
#include "host-common/YuvConverter.h"

// this is apple's video tool box header
#include <VideoToolbox/VideoToolbox.h>

#ifndef kVTVideoDecoderSpecification_RequireHardwareAcceleratedVideoDecoder
#define kVTVideoDecoderSpecification_RequireHardwareAcceleratedVideoDecoder \
    CFSTR("RequireHardwareAcceleratedVideoDecoder")
#endif

#include <cstdint>
#include <list>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include <stdio.h>
#include <string.h>

#include <stddef.h>

namespace android {
namespace emulation {

class MediaVideoToolBoxVideoHelper : public MediaVideoHelper {
public:
    enum class FrameStorageMode {
        USE_BYTE_BUFFER = 1,
        USE_GPU_TEXTURE = 2,
    };

    enum class OutputTreatmentMode {
        IGNORE_RESULT = 1,
        SAVE_RESULT = 2,
    };

    MediaVideoToolBoxVideoHelper(int w,
                                 int h,
                                 OutputTreatmentMode outMode,
                                 FrameStorageMode fMode);
    ~MediaVideoToolBoxVideoHelper() override;

    // return true if success; false otherwise
    bool init() override;
    void decode(const uint8_t* frame,
                size_t szBytes,
                uint64_t inputPts) override;
    void flush() override;
    void deInit() override;

    void resetTexturePool(MediaTexturePool* pool = nullptr) {
        mTexturePool = pool;
    }

    virtual int error() const override { return mErrorCode; }
    virtual bool good() const override { return mIsGood; }
    virtual bool fatal() const override { return false; }

private:
    // static
    static void videoToolboxDecompressCallback(void* opaque,
                                               void* sourceFrameRefCon,
                                               OSStatus status,
                                               VTDecodeInfoFlags flags,
                                               CVPixelBufferRef image_buffer,
                                               CMTime pts,
                                               CMTime duration);
    static CFDictionaryRef createOutputBufferAttributes(int width,
                                                        int height,
                                                        OSType pix_fmt);
    static CMSampleBufferRef createSampleBuffer(CMFormatDescriptionRef fmtDesc,
                                                void* buffer,
                                                size_t sz);

    void copyFrame();
    void copyFrameToTextures();
    void copyFrameToCPU();
    void createCMFormatDescription();
    void recreateDecompressionSession();
    void getOutputWH();
    void resetDecoderSession();
    void resetFormatDesc();

    struct InputFrame {
        InputFrame(H264NaluParser::H264NaluType in_type,
                   const uint8_t* in_data,
                   int in_size)
            : type(in_type), data(in_data), size(in_size) {}

        H264NaluParser::H264NaluType type;
        const uint8_t* data;
        int size;
    };

    std::vector<InputFrame> mInputFrames;

    bool parseInputFrames(const uint8_t* frame, size_t sz);

    // returns the remaining part of the frame, nullptr if none
    const uint8_t* parseOneFrame(const uint8_t* frame, size_t szBytes);

    void handleIDRFrame(const uint8_t* ptr, size_t szBytes, uint64_t pts);

    std::vector<uint8_t> mSPS;  // sps NALU
    std::vector<uint8_t> mPPS;  // pps NALU

    // turn on gpu texture mode
    bool mUseGpuTexture = true;
    MediaTexturePool* mTexturePool = nullptr;

    uint64_t mNumInputFrame{0};
    uint64_t mNumOutputFrame{0};
    int mErrorCode = 0;
    bool mIsGood = true;
    unsigned int mHeight = 0;
    unsigned int mWidth = 0;
    unsigned int mOutputHeight = 0;
    unsigned int mOutputWidth = 0;
    unsigned int mSurfaceHeight = 0;
    unsigned int mBPP = 0;
    unsigned int mSurfaceWidth = 0;
    unsigned int mLumaWidth = 0;
    unsigned int mLumaHeight = 0;
    unsigned int mChromaHeight = 0;
    unsigned int mOutBufferSize = 0;

    uint64_t mOutputPts = 0;
    bool mImageReady = false;

    // used in vtb callback to saved decoded frame
    std::vector<uint8_t> mSavedDecodedFrame;

    // TODO: get color aspects from some where
    MediaSnapshotState::ColorAspects mColorAspects{0};

    std::mutex mFrameLock;

    // this is only set to true after video session is created without errors
    // it is reset to false when new sps/pps comes
    bool mVtbReady = false;
    // videotoolbox stuff
    // the fmt, this will be recreated each time
    // we get a sps+pps frames
    CMFormatDescriptionRef mCmFmtDesc = nullptr;
    // The VideoToolbox decoder session: this could fail to
    // create due to incompatible formats coming from android guest
    VTDecompressionSessionRef mDecoderSession = nullptr;
    // where the decoded frame is stored
    CVPixelBufferRef mDecodedFrame = nullptr;

    // need this ffmpeg helper to get the w/h/colorspace info
    // TODO: replace it with webrtc h264 parser once it is built
    // for all platforms
    std::unique_ptr<MediaFfmpegVideoHelper> mFfmpegVideoHelper;
    void extractFrameInfo();

    uint64_t mTotalFrames = 0;
    // vtb decoder does not reorder output frames, that means
    // the video could see jumps all the times
    int mVtbBufferSize = 8;
    using PtsPair = std::pair<uint64_t, uint64_t>;
    std::map<PtsPair, MediaSnapshotState::FrameInfo> mVtbBufferMap;

};  // MediaVideoToolBoxVideoHelper

}  // namespace emulation
}  // namespace android
