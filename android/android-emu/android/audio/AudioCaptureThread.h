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

#include "android/base/threads/Thread.h"

namespace android {
namespace audio {

// Class to quickly read the audio data from the main loop thread
// and make it available for a user-supplied callback on a different thread.
class AudioCaptureThread : public android::base::Thread {
public:
    // Type of function that is called to transfer the content of a new
    // AudioPacket from the main thread to this thread.
    // |buf| is the audio data and
    // |size| is the size of |buf|.
    // |timestamp| is the timestamp(microsec) of the AudioPacket.
    using Callback =
            std::function<int(const void* buf, int size, uint64_t timestamp)>;

    // Stop the Audio engine and signal the thread to finish.
    virtual void stop() = 0;

    // Destructor
    virtual ~AudioCaptureThread() {}

    // Create a new AudioCaptureThread instance. |cb| is a function that
    // will be called from this thread, opaque is a handle to user data.
    static AudioCaptureThread* create(Callback cb,
                                      int sampleRate,
                                      int nbSamples,
                                      int bits,
                                      int nchannels);

protected:
    AudioCaptureThread() {}
};

}  // namespace audio
}  // namespace android
