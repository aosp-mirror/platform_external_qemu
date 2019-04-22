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
#include <string>

#include "VideoShareCapture.h"
#include "VideoShareInfo.h"
#include "api/scoped_refptr.h"
#include "media/engine/webrtcvideocapturer.h"
#include "media/engine/webrtcvideocapturerfactory.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_factory.h"

using DeviceInfo = webrtc::VideoCaptureModule::DeviceInfo;

namespace emulator {
namespace webrtc {

namespace videocapturemodule {

// A Factory that can be used by the WebRTC module to produce the
// proper video module that retrieves frames from shared memory.
class VideoShareFactory : public cricket::WebRtcVcmFactoryInterface {
public:
    VideoShareFactory(std::string handle, int32_t desiredFps);
    ~VideoShareFactory() = default;

    rtc::scoped_refptr<::webrtc::VideoCaptureModule> Create(
            const char* device) override;
    DeviceInfo* CreateDeviceInfo() override;
    void DestroyDeviceInfo(DeviceInfo* info) override;

private:
    bool mValid;
    std::string mMemoryHandle;
    VideoShareInfo mShareInfo;
};

}  // namespace videocapturemodule
}  // namespace webrtc
}  // namespace emulator
