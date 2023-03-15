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
#include "android/emulation/control/utils/AudioUtils.h"

#include "emulator_controller.pb.h"  // for AudioFormat, AudioFormat::AUD_FM...

namespace android {
namespace emulation {
namespace control {

android::recording::AudioFormat AudioUtils::getSampleFormat(
        const AudioFormat& fmt) {
    switch (fmt.format()) {
        case AudioFormat::AUD_FMT_S16:
            return android::recording::AudioFormat::AUD_FMT_S16;
        case AudioFormat::AUD_FMT_U8:
            return android::recording::AudioFormat::AUD_FMT_U8;
        default:
            return android::recording::AudioFormat::AUD_FMT_S16;
    }
}

int AudioUtils::getChannels(const AudioFormat& fmt) {
    switch (fmt.channels()) {
        case AudioFormat::Mono:
            return 1;
        case AudioFormat::Stereo:
            return 2;
        default:
            return 2;
    }
}

int AudioUtils::getBytesPerSample(const AudioFormat& fmt) {
    return fmt.format() == AudioFormat::AUD_FMT_U8 ? 1 : 2;
}

}  // namespace control
}  // namespace emulation
}  // namespace android
