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

#include "android/emulation/MediaH264Decoder.h"
#include "android/emulation/H264NaluParser.h"

#include <cstdint>
#include <string>
#include <vector>

#include <stdio.h>
#include <string.h>

// cuda and nvidia decoder related headers
#include <cuda.h>
#include <nvcuvid.h>

#define MEDIA_H264_DEBUG 1

#if MEDIA_H264_DEBUG
#define H264_DPRINT(fmt,...) fprintf(stderr, "h264-dec: %s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define H264_DPRINT(fmt,...)
#endif

namespace android {
namespace emulation {

namespace {

class MediaH264DecoderImpl : public MediaH264Decoder {
public:
    MediaH264DecoderImpl() = default;
    virtual ~MediaH264DecoderImpl();

    // This is the entry point
    virtual void handlePing(MediaCodecType type, MediaOperation op, void* ptr) override;

private:
    virtual void initH264Context(unsigned int width,
                                 unsigned int height,
                                 unsigned int outWidth,
                                 unsigned int outHeight,
                                 PixelFormat pixFmt) override;
    virtual void destroyH264Context() override;
    virtual void decodeFrame(void* ptr, const uint8_t* frame, size_t szBytes) override;
    virtual void flush(void* ptr) override;
    virtual void getImage(void* ptr) override;

public:
    // cuda related methods
    static bool initCudaDrivers();
    static bool s_isCudaInitialized;

    //
}; // MediaH264DecoderImpl

MediaH264DecoderImpl::~MediaH264DecoderImpl() {
}

void MediaH264DecoderImpl::handlePing(MediaCodecType type, MediaOperation op, void* ptr) {
    H264_DPRINT("NOT IMPLEMENTED");
}

void MediaH264DecoderImpl::initH264Context(unsigned int width,
                             unsigned int height,
                             unsigned int outWidth,
                             unsigned int outHeight,
                             PixelFormat pixFmt) {
    H264_DPRINT("NOT IMPLEMENTED");
}

void MediaH264DecoderImpl::destroyH264Context() {
    H264_DPRINT("NOT IMPLEMENTED");
}

void MediaH264DecoderImpl::decodeFrame(void* ptr, const uint8_t* frame, size_t szBytes) {
    H264_DPRINT("NOT IMPLEMENTED");
}

void MediaH264DecoderImpl::flush(void* ptr) {
    H264_DPRINT("NOT IMPLEMENTED");
}

void MediaH264DecoderImpl::getImage(void* ptr) {
    H264_DPRINT("NOT IMPLEMENTED");
}

bool MediaH264DecoderImpl::initCudaDrivers() {
    if (s_isCudaInitialized) {
      return true;
    }

    // this should be called at the very beginning, before we call anything else
    CUresult initResult = cuInit(0);
    if (initResult != CUDA_SUCCESS) {
        H264_DPRINT("Failed to init cuda drivers error code %d", (int)initResult);
        s_isCudaInitialized = false;
        return false;
    }

    int numGpuCards = 0;
    CUresult myres = cuDeviceGetCount(&numGpuCards);
    if (myres != CUDA_SUCCESS) {
        H264_DPRINT("Failed to get number of GPU cards installed on host; error code %d", (int)myres);
        return false;
    }

    if (numGpuCards <= 0) {
        H264_DPRINT("There are no nvidia GPU cards on this host.");
        return false;
    }

    // lukily, we get cuda initialized.
    s_isCudaInitialized = true;

    return true;
}

};  // namespace

bool MediaH264DecoderImpl::s_isCudaInitialized = false;
// static
MediaH264Decoder* MediaH264Decoder::create() {
    if(!MediaH264DecoderImpl::initCudaDrivers()) {
      return nullptr;
    }
    return new MediaH264DecoderImpl();
}

}  // namespace emulation
}  // namespace android

