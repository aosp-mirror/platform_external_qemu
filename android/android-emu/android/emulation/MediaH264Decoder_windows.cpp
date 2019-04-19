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
 
};  // namespace

// static
MediaH264Decoder* MediaH264Decoder::create() {
    return new MediaH264DecoderImpl();
}

}  // namespace emulation
}  // namespace android

