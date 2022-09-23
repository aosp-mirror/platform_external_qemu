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
#pragma once

#include "android/recording/AvFormat.h"  // for AudioFormat

namespace android {
namespace emulation {
namespace control {
class AudioFormat;

// Contains a set of convenience methods to assist with streaming audio.
class AudioUtils {
public:
    // Translates our external format to the internal format.
    static android::recording::AudioFormat getSampleFormat(const AudioFormat& fmt);

    // Extracts the number of channels from the enum for usage with qemu.
    static int getChannels(const AudioFormat& fmt);

    // Number of bytes in a sample
    static int getBytesPerSample(const AudioFormat& fmt);
};

}  // namespace control
}  // namespace emulation
}  // namespace android
