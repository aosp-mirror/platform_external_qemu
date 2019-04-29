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
#include "VideoShareFactory.h"
namespace emulator {
namespace webrtc {

namespace videocapturemodule {

VideoShareFactory::VideoShareFactory(std::string handle, int32_t desiredFps)
    : mMemoryHandle(handle), mShareInfo(VideoShareInfo(handle, desiredFps)) {
    mValid = mShareInfo.Init() == 0;
}
rtc::scoped_refptr<::webrtc::VideoCaptureModule> VideoShareFactory::Create(
        const char* device) {
    if (!mValid)
        return nullptr;

    rtc::scoped_refptr<VideoShareCapture> implementation(
            new rtc::RefCountedObject<VideoShareCapture>(
                    mShareInfo.capability()));
    if (implementation->Init(mMemoryHandle) != 0)
        return nullptr;

    return implementation;
};

DeviceInfo* VideoShareFactory::CreateDeviceInfo() {
    return &mShareInfo;
}

void VideoShareFactory::DestroyDeviceInfo(DeviceInfo* info) {}

}  // namespace videocapturemodule
}  // namespace webrtc
}  // namespace emulator
