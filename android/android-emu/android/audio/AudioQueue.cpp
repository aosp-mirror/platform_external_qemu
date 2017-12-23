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

#include "android/audio/AudioQueue.h"

#include "android/base/synchronization/Lock.h"

namespace android {
namespace audio {

using android::base::AutoLock;
using android::base::Lock;

namespace {
// Real implementation of AudioQueue class
class AudioQueueImpl : public AudioQueue {
public:
    AudioQueueImpl() {

    }

    // Get an audio packet from the pool.
    AudioPacket* acquire() override {
    }

    // Release the audio packet back into the pool.
    void release(AudioPacket* pkt) override {

    }

    // Locked version of acquire().
    AudioPacket* acquireLocked() override {
        
    }

    // Locked version of release().
    void releaseLocked(AudioPacket* pkt) override {

    }

    // Get number of AudioPackets in the pool
    size_t size() override {
        return mPool.size();
    }

    // Get capacity of the pool
    size_t capacity() override {
        return mPool.capacity();
    }

    // Check if pool is empty
    bool empty() override {
        return mPool.empty();
    }

    // Destructor
    virtual ~AudioQueue() {}

private:
    std::vector<AudioPacket> mPool;
    android::base::Lock mLock;
};

}  // namespace

AudioQueue* AudioQueue::create() {
    return new AudioQueueImpl();
}
}  // namespace audio
}  // namespace android


