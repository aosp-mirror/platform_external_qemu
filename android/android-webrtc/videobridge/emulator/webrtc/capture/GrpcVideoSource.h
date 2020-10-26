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

#include <absl/types/optional.h>                    // for optional
#include <api/media_stream_interface.h>             // for MediaSourceInterf...
#include <api/scoped_refptr.h>                      // for scoped_refptr
#include <grpcpp/grpcpp.h>                          // for ClientContext
#include <media/base/adapted_video_track_source.h>  // for AdaptedVideoTrack...
#include <stdint.h>                                 // for uint32_t

#include <string>  // for string
#include <thread>  // for thread

#include "emulator/net/EmulatorGrcpClient.h"  // for EmulatorGrpcClient

namespace android {
namespace emulation {
namespace control {
class Image;
}  // namespace control
}  // namespace emulation
}  // namespace android
namespace webrtc {
class I420Buffer;
}  // namespace webrtc

namespace emulator {
namespace webrtc {

using ::android::emulation::control::Image;
class GrpcVideoSource : public rtc::AdaptedVideoTrackSource {
public:
    explicit GrpcVideoSource(std::string discovery_file);
    ~GrpcVideoSource() override;

    bool is_screencast() const override { return true; }
    absl::optional<bool> needs_denoising() const override { return false; }
    SourceState state() const override {
        return ::webrtc::MediaSourceInterface::SourceState::kLive;
    }
    bool remote() const override { return false; }
    void setFps(uint32_t fps);

private:
    // Listens for available audio packets from the Android Emulator.
    void captureFrames();

    grpc::ClientContext mContext;
    EmulatorGrpcClient mClient;

    rtc::scoped_refptr<::webrtc::I420Buffer>
            mI420Buffer;  // Re-usable I420Buffer.
    std::thread mVideoThread;
    bool mCaptureVideo{true};
};
}  // namespace webrtc
}  // namespace emulator
