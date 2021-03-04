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

#include "android/emulation/MediaVideoToolBoxVideoHelper.h"
#include "android/emulation/YuvConverter.h"
#include "android/utils/debug.h"

extern "C" {
}

#define MEDIA_VTB_DEBUG 0

#if MEDIA_VTB_DEBUG
#define VTB_DPRINT(fmt, ...)                                             \
    fprintf(stderr, "media-vtb-video-helper: %s:%d " fmt "\n", __func__, \
            __LINE__, ##__VA_ARGS__);
#else
#define VTB_DPRINT(fmt, ...)
#endif


namespace android {
namespace emulation {

using TextureFrame = MediaTexturePool::TextureFrame;
using FrameInfo = MediaSnapshotState::FrameInfo;
using ColorAspects = MediaSnapshotState::ColorAspects;

MediaVideoToolBoxVideoHelper::MediaVideoToolBoxVideoHelper(OutputTreatmentMode oMode,
                                           FrameStorageMode fMode)
    : mUseGpuTexture(fMode == FrameStorageMode::USE_GPU_TEXTURE) {
    mIgnoreDecoderOutput = (oMode == OutputTreatmentMode::IGNORE_RESULT);
}

MediaVideoToolBoxVideoHelper::~MediaVideoToolBoxVideoHelper() {
    deInit();
}

void MediaVideoToolBoxVideoHelper::deInit() {
    VTB_DPRINT("deInit calling");

    mSavedDecodedFrames.clear();
}

bool MediaVideoToolBoxVideoHelper::init() {
    return true;
}

void MediaVideoToolBoxVideoHelper::decode(const uint8_t* frame,
                                  size_t szBytes,
                                  uint64_t inputPts) {
    VTB_DPRINT("%s(frame=%p, sz=%zu)", __func__, frame, szBytes);

}

void MediaVideoToolBoxVideoHelper::flush() {
    VTB_DPRINT("started flushing");
    VTB_DPRINT("done one flushing");
}

}  // namespace emulation
}  // namespace android
