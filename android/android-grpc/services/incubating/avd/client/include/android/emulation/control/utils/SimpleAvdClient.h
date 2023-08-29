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

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>

#include "absl/status/statusor.h"
#include "android/emulation/control/utils/EmulatorGrcpClient.h"
#include "avd_service.grpc.pb.h"
#include "avd_service.pb.h"

namespace android {
namespace emulation {
namespace control {

template <typename T>
using OnCompleted = std::function<void(absl::StatusOr<T*>)>;

using namespace incubating;
using ::google::protobuf::Empty;

class SimpleAvdClient {
public:
    explicit SimpleAvdClient(std::shared_ptr<EmulatorGrpcClient> client)
        : mClient(client), mService(client->stub<AvdService>()) {}

    // Get the avd info from the emulator, caching the value once it has
    // been received.
    void getAvdInfo(OnCompleted<AvdInfo> status);

private:
    std::shared_ptr<EmulatorGrpcClient> mClient;
    std::unique_ptr<AvdService::Stub> mService;
    std::atomic_bool mLoaded;
    AvdInfo mCachedInfo;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
