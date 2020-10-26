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

#include <absl/types/optional.h>              // for optional
#include <api/media_stream_interface.h>       // for MediaSourceInterface::S...
#include <api/notifier.h>                     // for Notifier
#include <api/scoped_refptr.h>                // for scoped_refptr
#include <grpcpp/grpcpp.h>                    // for ClientContext
#include <media/base/video_adapter.h>         // for VideoAdapter
#include <media/base/video_broadcaster.h>     // for VideoBroadcaster
#include <atomic>                             // for atomic_bool
#include <mutex>                              // for mutex
#include <string>                             // for string
#include <thread>                             // for thread

#include "emulator/net/EmulatorGrcpClient.h"  // for EmulatorGrpcClient

namespace rtc {
struct VideoSinkWants;
template <typename VideoFrameT> class VideoSinkInterface;
}  // namespace rtc

namespace android {
namespace emulation {
namespace control {
class Image;
}  // namespace control
}  // namespace emulation
}  // namespace android
namespace webrtc {
class I420Buffer;
class RecordableEncodedFrame;
class VideoFrame;
}  // namespace webrtc

namespace emulator {
namespace webrtc {

using ::android::emulation::control::Image;
class GrpcVideoSource
    : public ::webrtc::Notifier<::webrtc::VideoTrackSourceInterface> {
public:
    explicit GrpcVideoSource(std::string discovery_file);
    ~GrpcVideoSource() override;

    bool is_screencast() const override { return true; }
    absl::optional<bool> needs_denoising() const override { return false; }
    SourceState state() const override {
        return ::webrtc::MediaSourceInterface::SourceState::kLive;
    }
    bool remote() const override { return false; }

private:
    // Implements rtc::VideoSourceInterface.
    void AddOrUpdateSink(rtc::VideoSinkInterface<::webrtc::VideoFrame>* sink,
                         const rtc::VideoSinkWants& wants) override;
    void RemoveSink(rtc::VideoSinkInterface<::webrtc::VideoFrame>* sink) override;

    // Part of VideoTrackSourceInterface.
    bool GetStats(Stats* stats) override;

    void OnSinkWantsChanged(const rtc::VideoSinkWants& wants);

    // Encoded sinks not implemented for AdaptedVideoTrackSource.
    bool SupportsEncodedOutput() const override { return false; }
    void GenerateKeyFrame() override {}
    void AddEncodedSink(
            rtc::VideoSinkInterface<::webrtc::RecordableEncodedFrame>* sink)
            override {}
    void RemoveEncodedSink(
            rtc::VideoSinkInterface<::webrtc::RecordableEncodedFrame>* sink)
            override {}

    void captureFrames();

    grpc::ClientContext mContext;
    EmulatorGrpcClient mClient;

    rtc::scoped_refptr<::webrtc::I420Buffer>
            mI420Buffer;  // Re-usable I420Buffer.
    std::thread mVideoThread;
    std::atomic_bool mCaptureVideo{false};

    cricket::VideoAdapter mVideoAdapter;

    std::mutex mStatsMutex;
    absl::optional<Stats> mStats;

    rtc::VideoBroadcaster mBroadcaster;
};
}  // namespace webrtc
}  // namespace emulator
