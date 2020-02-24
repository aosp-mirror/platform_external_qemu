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

#pragma once

#include "android/emulation/GoldfishMediaDefs.h"
#include "android/emulation/H264NaluParser.h"
#include "android/emulation/H264PingInfoParser.h"
#include "android/emulation/MediaCodec.h"
#include "android/emulation/MediaH264DecoderPlugin.h"
#include "android/emulation/MediaHostRenderer.h"

#include <VideoToolbox/VideoToolbox.h>

#include <cstdint>
#include <string>
#include <vector>

#include <stdio.h>
#include <string.h>

#ifndef kVTVideoDecoderSpecification_RequireHardwareAcceleratedVideoDecoder
#define kVTVideoDecoderSpecification_RequireHardwareAcceleratedVideoDecoder CFSTR("RequireHardwareAcceleratedVideoDecoder")
#endif


#include <stddef.h>

namespace android {
namespace emulation {

class MediaH264DecoderVideoToolBox : public MediaH264DecoderPlugin {
public:
    virtual void initH264Context(void* ptr) override;
    virtual void reset(void* ptr) override;
    virtual MediaH264DecoderPlugin* clone() override;
    virtual void destroyH264Context() override;
    virtual void decodeFrame(void* ptr) override;
    virtual void flush(void* ptr) override;
    virtual void getImage(void* ptr) override;

    virtual void save(base::Stream* stream) const override;
    virtual bool load(base::Stream* stream) override;

    virtual int type() const override { return PLUGIN_TYPE_VIDEO_TOOL_BOX; }


    explicit MediaH264DecoderVideoToolBox(uint64_t id,
                                          H264PingInfoParser parser);
    virtual ~MediaH264DecoderVideoToolBox();

public:
    enum class DecoderState {
        BAD_STATE = 0,
        GOOD_STATE = 1,
    };

    DecoderState getState() const {return mState;}
    std::vector<uint8_t> getSPS() const { return mSPS; }
    std::vector<uint8_t> getPPS() const { return mPPS; }
private:
    void initH264ContextInternal(unsigned int width,
                                 unsigned int height,
                                 unsigned int outWidth,
                                 unsigned int outHeight,
                                 PixelFormat pixFmt);
    uint64_t mId = 0;
    H264PingInfoParser mParser;
    MediaHostRenderer mRenderer;
    DecoderState mState = DecoderState::GOOD_STATE;

    void decodeFrameInternal(size_t* pRetSzBytes, int32_t* pRetErr, const uint8_t* frame, size_t szBytes, uint64_t pts, size_t consumedSzBytes);
    // Passes the Sequence Parameter Set (SPS) and Picture Parameter Set (PPS) to the
    // videotoolbox decoder
    CFDataRef createVTDecoderConfig();
    // Callback passed to the VTDecompressionSession
    static void videoToolboxDecompressCallback(void* opaque,
                                               void* sourceFrameRefCon,
                                               OSStatus status,
                                               VTDecodeInfoFlags flags,
                                               CVImageBufferRef image_buffer,
                                               CMTime pts,
                                               CMTime duration);
    static CFDictionaryRef createOutputBufferAttributes(int width,
                                                        int height,
                                                        OSType pix_fmt);
    static CMSampleBufferRef createSampleBuffer(CMFormatDescriptionRef fmtDesc,
                                                void* buffer,
                                                size_t sz);
    static OSType toNativePixelFormat(PixelFormat fmt);

    // We should move these shared memory calls elsewhere, as vpx decoder is also using the same/similar
    // functions
    static void* getReturnAddress(void* ptr);
    static uint8_t* getDst(void* ptr);

    void handleIDRFrame(const uint8_t* ptr, size_t szBytes, uint64_t pts);
    void handleNonIDRFrame(const uint8_t* ptr, size_t szBytes, uint64_t pts);
    void handleSEIFrame(const uint8_t* ptr, size_t szBytes);

    void createCMFormatDescription();
    void recreateDecompressionSession();
    
    // The VideoToolbox decoder session
    VTDecompressionSessionRef mDecoderSession = nullptr;
    // The decoded video buffer
    uint64_t mOutputPts = 0;
    CVImageBufferRef mDecodedFrame = nullptr;
    CMFormatDescriptionRef mCmFmtDesc = nullptr;
    bool mImageReady = false;
    static constexpr int kBPP = 2; // YUV420 is 2 bytes per pixel
    unsigned int mOutputHeight = 0;
    unsigned int mOutputWidth = 0;
    unsigned int mWidth = 0;
    unsigned int mHeight = 0;
    PixelFormat mOutPixFmt;
    // The calculated size of the outHeader buffer size allocated in the guest.
    // It should be sizeY + (sizeUV * 2), where:
    //  sizeY = outWidth * outHeight,
    //  sizeUV = sizeY / 4
    // It is equivalent to outWidth * outHeight * 3 / 2
    unsigned int mOutBufferSize = 0;
    std::vector<uint8_t> mSPS; // sps NALU
    std::vector<uint8_t> mPPS; // pps NALU

    bool mIsInFlush = false;
private:

    bool mIsLoadingFromSnapshot = false;
    std::vector<uint8_t> mSavedDecodedFrame;
    void copyFrame();
    void oneShotDecode(std::vector<uint8_t> & data, uint64_t pts);

    mutable SnapshotState  mSnapshotState;

}; // MediaH264DecoderVideoToolBox


}  // namespace emulation
}  // namespace android
