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
#include "emulator/webrtc/capture/VideoCapturerFactory.h"

#include <string>                                        // for string, oper...
#include <unordered_map>                                 // for operator!=
#include <utility>                                       // for pair

#include "emulator/webrtc/capture/VideoCapturer.h"       // for VideoCapturer
#include "emulator/webrtc/capture/VideoShareCapturer.h"  // for VideoShareCa...

namespace emulator {
namespace webrtc {

VideoCapturer* VideoCapturerFactory::getVideoCapturer(std::string handle) {
    auto entry = mHandleToCapturers.find(handle);
    if (entry != mHandleToCapturers.end()) {
        return entry->second.get();
    }

    auto capturer = new VideoShareCapturer(handle);
    mHandleToCapturers[handle] = std::unique_ptr<VideoCapturer>(capturer);
    return capturer;
}

}  // namespace webrtc
}  // namespace emulator
