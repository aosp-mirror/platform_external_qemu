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

#include "android/emulation/MediaFrameRateCounter.h"

#include <stdio.h>
#include <string.h>

#define MEDIA_VPX_DEBUG 1

#if MEDIA_VPX_DEBUG
#define VPX_DPRINT(fmt, ...)                                        \
    fprintf(stderr, "vpx-dec: %s:%d " fmt "\n", __func__, __LINE__, \
            ##__VA_ARGS__);
#else
#define VPX_DPRINT(fmt, ...)
#endif

namespace android {
namespace emulation {

void MediaFrameRateCounter::start() {
    mStartTime = std::chrono::steady_clock::now();
    mState = State::STATE_STARTED;
    mFrameCount = 0;
}

void MediaFrameRateCounter::stop() {
    mStopTime = std::chrono::steady_clock::now();
    mState = State::STATE_STOPPED;
}

void MediaFrameRateCounter::addOneFrame() {
    ++mFrameCount;
}

double MediaFrameRateCounter::frameRate() const {
    if (mState == State::STATE_NOT_STARTED) {
        return 0.0;
    }

    int mycount = 0;
    if (mState == State::STATE_STOPPED) {
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                mStopTime - mStartTime);
        mycount = elapsed.count();
    } else if (mState == State::STATE_STARTED) {
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - mStartTime);
        mycount = elapsed.count();
    }
    if (mycount <= 0) {
        return 0;
    }
    return (mFrameCount * S_TO_US) / mycount;
}

}  // namespace emulation
}  // namespace android
