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
#include "modules/video_capture/video_capture.h"

namespace emulator {
namespace webrtc {
namespace videocapturemodule {

// Provides information about the "virtual" device that obtains
// images from a shared memory handle. It will always deliver
// frames in the I420 format, as that's what you will find in the
// shared memory region.
//
// Note: We currently are not using the fps set in the shared memory
// region.
//
// The shared memory region from the emulator should look as follows:
//
//      uint32_t width;
//      uint32_t height;
//      uint32_t fps;
//      uint8_t[width*height*12/8] pixels;
class VideoShareInfo : public ::webrtc::VideoCaptureModule::DeviceInfo {
public:
    struct VideoInfo {
        uint32_t width;
        uint32_t height;
        uint32_t fps;          // Target framerate.
        uint32_t frameNumber;  // Frame number.
        uint64_t tsUs;         // Timestamp when this frame was received.
    };

    VideoShareInfo(std::string handle, int fps) : mHandle(handle), mFps(fps) {}
    ~VideoShareInfo() override = default;

    int32_t Init();
    uint32_t NumberOfDevices() override { return 1; }
    int32_t GetDeviceName(uint32_t deviceNumber,
                          char* deviceNameUTF8,
                          uint32_t deviceNameLength,
                          char* deviceUniqueIdUTF8,
                          uint32_t deviceUniqueIdUTF8Length,
                          char* productUniqueIdUTF8 = 0,
                          uint32_t productUniqueIdUTF8Length = 0) override;

    int32_t NumberOfCapabilities(const char* deviceName) override { return 1; }

    int32_t GetCapability(
            const char* deviceUniqueIdUTF8,
            const uint32_t deviceCapabilityNumber,
            ::webrtc::VideoCaptureCapability& capability) override {
        capability = mCapability;
        return 0;
    }

    int32_t GetOrientation(const char* deviceUniqueIdUTF8,
                           ::webrtc::VideoRotation& orientation) override {
        orientation = ::webrtc::VideoRotation::kVideoRotation_0;
        return 0;
    }

    int32_t GetBestMatchedCapability(
            const char* deviceUniqueIdUTF8,
            const ::webrtc::VideoCaptureCapability& requested,
            ::webrtc::VideoCaptureCapability& resulting) override {
        resulting = mCapability;
        return 0;
    }

    int32_t DisplayCaptureSettingsDialogBox(const char* /*deviceUniqueIdUTF8*/,
                                            const char* /*dialogTitleUTF8*/,
                                            void* /*parentWindow*/,
                                            uint32_t /*positionX*/,
                                            uint32_t /*positionY*/) override {
        return -1;
    }

    ::webrtc::VideoCaptureCapability capability() const { return mCapability; }

private:
    ::webrtc::VideoCaptureCapability mCapability;
    std::string mHandle;
    int32_t mFps;
};
}  // namespace videocapturemodule
}  // namespace webrtc
}  // namespace emulator
