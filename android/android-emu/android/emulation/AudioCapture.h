// Copyright (C) 2017 The Android Open Source Project
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

#include "android/base/Compiler.h"

namespace android {
namespace emulation {

class AudioCapturer {
public:
    explicit AudioCapturer(int freq, int bits, int nchannels) :
    mSamplingRate(freq),
    mBits(bits),
    mChannels(nchannels)
    {
    }

    virtual ~AudioCapturer() {}

    // This is the audio stream received from the capturer, subclass mush override this method
    virtual int onSample(void *buf, int size) = 0;

    // Return the sampling rate for the audio, usually 44100 or 48000
    int getSamplingRate() const {
        return mSamplingRate;
    }

    // Return number of bits per channel, usually 16- or 8- bits
    int getBits() const {
        return mBits;
    }

    // return number of channel per frame, usually 2 for stero audio, and 1 for mono
    int getChannels() const {
        return mChannels;
    }

private:

    int mSamplingRate;
    int mBits;
    int mChannels;

    DISALLOW_COPY_AND_ASSIGN(AudioCapturer);
};

}  // namespace emulation
}  // namespace android
