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
#include <memory>         // for unique_ptr
#include <string>         // for string, hash, operator==
#include <unordered_map>  // for unordered_map

#include "emulator/webrtc/capture/VideoCapturer.h"
namespace emulator {
namespace webrtc {

class VideoCapturerFactory {
public:
    VideoCapturer* getVideoCapturer(std::string handle);

private:
    std::unordered_map<std::string, std::unique_ptr<VideoCapturer>>
            mHandleToCapturers;
};

}  // namespace webrtc
}  // namespace emulator
