// Copyright (C) 2023 The Android Open Source Project
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

#include <functional>
#include <memory>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "android/emulation/control/utils/EmulatorGrcpClient.h"
#include "google/protobuf/empty.pb.h"
#include "screen_recording_service.grpc.pb.h"
#include "screen_recording_service.pb.h"

namespace android {
namespace emulation {
namespace control {

template <typename T>
using OnCompleted = std::function<void(absl::StatusOr<T*>)>;
using OnFinished = std::function<void(absl::Status)>;

template <typename T>
using OnEvent = std::function<void(const T*)>;
using namespace incubating;
using ::google::protobuf::Empty;

/**
 * @brief A client for interacting with the Screen Recording Service.
 */
class SimpleScreenRecordingClient {
public:
    explicit SimpleScreenRecordingClient(
            std::shared_ptr<EmulatorGrpcClient> client)
        : mClient(client), mService(client->stub<ScreenRecording>()) {}

    // TODO(jansene): Add methods when needed.
private:
    std::shared_ptr<EmulatorGrpcClient> mClient;
    std::unique_ptr<ScreenRecording::Stub> mService;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
