// Copyright (C) 2018 The Android Open Source Project
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

#include "android/emulation/control/display_agent.h"
#include "android/recording/Frame.h"

namespace android {
namespace recording {

// Helper class to readback guest video frames. The posted frames are
// triple-buffered to allow a consumer to read the latest frame while new
// results are using a double-buffered solution, which should reduce the blocked
// time for both the producer and consumer.
class GuestReadbackWorker {
public:
    // Returns the latest posted frame, or null if no data has been posted yet.
    // This call is okay to be blocked by the caller, since the post call is
    // writing to the other two buffers. The timestamp of the frame will be set
    // to the current time upon calling this function.
    virtual const Frame* getFrame() = 0;

    // Returns the pixel format
    virtual VideoFormat getPixelFormat() = 0;

    // Destructor
    virtual ~GuestReadbackWorker() {}

    // Create a new AudioCaptureThread instance. |cb| is a function that
    // will be called from this thread with new audio frames.
    static std::unique_ptr<GuestReadbackWorker> create(
            const QAndroidDisplayAgent* agent,
            uint32_t fbWidth,
            uint32_t fbHeight);

protected:
    GuestReadbackWorker() {}
};

}  // namespace recording
}  // namespace android
