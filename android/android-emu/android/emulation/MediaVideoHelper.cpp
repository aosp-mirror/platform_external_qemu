// Copyright (C) 2020 The Android Open Source Project
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

#include "android/emulation/MediaVideoHelper.h"

#define MEDIA_VIDEO_DEBUG 0

#if MEDIA_VIDEO_DEBUG
#define MEDIA_DPRINT(fmt, ...)                                                 \
    fprintf(stderr, "media-video-helper: %s:%d " fmt "\n", __func__, __LINE__, \
            ##__VA_ARGS__);
#else
#define MEDIA_DPRINT(fmt, ...)
#endif

namespace android {
namespace emulation {

using FrameInfo = MediaSnapshotState::FrameInfo;

bool MediaVideoHelper::receiveFrame(FrameInfo* pFrameInfo) {
    if (mSavedDecodedFrames.empty()) {
        MEDIA_DPRINT("no frame to return");
        return false;
    }
    std::swap(*pFrameInfo, mSavedDecodedFrames.front());
    mSavedDecodedFrames.pop_front();
    MEDIA_DPRINT("has frame to return");
    return true;
}

}  // namespace emulation
}  // namespace android
