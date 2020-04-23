// Copyright (C) 2015 The Android Open Source Project
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

#include <cstdint>
#include "android/base/async/Looper.h"

class Looper;
namespace android {

namespace base {
class Looper;
}  // namespace base

namespace opengl {

// GpuFrameBridge is a helper class to forward Gpu frame to its clients.
// Usage is the following:
// 1) Create a new GpuFrameBridge instance.
// 2) Register the FrameAvailableCallback if needed.
// 3) Call getRecordFrame or getRecordFrameAsync to receive frame.
class GpuFrameBridge {
public:
    // Create a new GpuFrameBridge instance.
    static GpuFrameBridge* create();

    // Destructor
    virtual ~GpuFrameBridge() {}

    // Post callback (synchronous) specifically for recording purposes.
    virtual void postRecordFrame(int width, int height, const void* pixels) = 0;

    // Async version of postRecordFrame for use with async readback.
    // Does not read the frame immediately.
    virtual void postRecordFrameAsync(int width,
                                      int height,
                                      const void* pixels) = 0;

    // Returns the currently displayed frame. This method is designed only for
    // recording. Returns null if there is no frame available. Make sure to
    // attach the postFrameRecording() as the callback or you will not get a
    // valid frame.
    virtual void* getRecordFrame() = 0;

    // Async version of getRecordFrame.
    virtual void* getRecordFrameAsync() = 0;

    // Invalidates the recording buffers. Once called, getRecordFrame() and it's
    // async version will return null until new data has been posted.
    virtual void invalidateRecordingBuffers() = 0;

    typedef void (*FrameAvailableCallback)(void* opaque);

    virtual void setFrameReceiver(FrameAvailableCallback receiver, void* opaque) = 0;

    virtual void setDisplayId(uint32_t displayId) = 0;

    virtual void setLooper(android::base::Looper* aLooper) = 0;

protected:
    GpuFrameBridge() {}
    GpuFrameBridge(const GpuFrameBridge& other);
};

}  // namespace opengl
}  // namespace android
