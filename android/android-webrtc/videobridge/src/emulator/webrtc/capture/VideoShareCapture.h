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

#include "rtc_base/platform_thread.h"
// #include "video_capture_impl2.h"
#include "modules/video_capture/video_capture_impl.h"
#include "emulator/base/memory/SharedMemory.h"

using emulator::base::SharedMemory;

using ::webrtc::VideoCaptureCapability;

namespace emulator {
namespace webrtc {

namespace videocapturemodule {
// We should subclass from VideoCaptureImpl. However we get the following:
// error: undefined symbol: typeinfo for webrtc::videocapturemodule::VideoCaptureImpl
class VideoShareCapture : public ::webrtc::videocapturemodule::VideoCaptureImpl {
 public:
  VideoShareCapture(VideoCaptureCapability settings)
      : mSettings(settings), mSharedMemory("", 0) {}
  ~VideoShareCapture() {}

  int32_t Init(std::string handle);
  int32_t StartCapture(const VideoCaptureCapability& capability) override;
  int32_t StopCapture() override;
  bool CaptureStarted() override { return mCaptureStarted; }
  int32_t CaptureSettings(VideoCaptureCapability& settings) override {
    settings = mSettings;
    return 0;
  }

 private:
  static bool CaptureThread(void* obj) {
    return static_cast<VideoShareCapture*>(obj)->CaptureProcess();
  }

  bool CaptureProcess();

  std::unique_ptr<rtc::PlatformThread> mCaptureThread;
  rtc::CriticalSection mCaptureCS;
  VideoCaptureCapability mSettings;
  SharedMemory mSharedMemory;
  void* mPixelBuffer;
  uint32_t mPixelBufferSize;
  bool mCaptureStarted;
};

}  // namespace videocapturemodule
}  // namespace webrtc
}
