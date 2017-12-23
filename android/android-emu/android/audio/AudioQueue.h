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

#include "android/audio/AudioPacket.h"

namespace android {
namespace audio {

// Class to reuse audio frames that were previously allocated so we can
// remove the time used to allocate and delete.
class AudioQueue {
public:
    // Get an audio packet from the pool.
    virtual AudioPacket* acquire() = 0;

    // Release the audio packet back into the pool.
    virtual void release(AudioPacket* pkt) = 0;

    // Locked version of acquire().
    virtual AudioPacket* acquireLocked() = 0;

    // Locked version of release().
    virtual void releaseLocked(AudioPacket* pkt) = 0;

    // Get number of AudioPackets in the pool
    virtual size_t size() = 0;

    // Get capacity of the pool
    virtual size_t capacity() = 0;

    // Check if pool is empty
    virtual bool empty() = 0;

    // Destructor
    virtual ~AudioQueue() {}

    // Create a new AudioQueue instance.
    static AudioQueue* create();

protected:
    AudioQueue() {}

};

}  // namespace audio
}  // namespace android

