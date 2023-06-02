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
#include <string>
#include <type_traits>

#include <grpcpp/grpcpp.h>
#include "absl/status/statusor.h"
#include "android/emulation/control/utils/EmulatorGrcpClient.h"

#include "virtual_scene_service.grpc.pb.h"
#include "virtual_scene_service.pb.h"

namespace android {
namespace emulation {
namespace control {

template <typename T>
using OnCompleted = std::function<void(absl::StatusOr<T*>)>;

using android::emulation::control::incubating::AnimationState;
using android::emulation::control::incubating::Poster;
using android::emulation::control::incubating::PosterList;
using android::emulation::control::incubating::VirtualSceneService;

/**
 * @brief A client for interacting with the VirtualScene Service.
 */
class SimpleVirtualSceneServiceClient {
public:
    explicit SimpleVirtualSceneServiceClient(
            std::shared_ptr<EmulatorGrpcClient> client)
        : mClient(client), mService(client->stub<VirtualSceneService>()) {}

    void listPostersAsync(OnCompleted<PosterList> onDone);
    void setPosterAsync(Poster poster, OnCompleted<Poster> onDone);

    void setAnimationState(AnimationState state,
                           OnCompleted<AnimationState> onDone);
    void getAnimationState(OnCompleted<AnimationState> onDone);

private:
    std::shared_ptr<EmulatorGrpcClient> mClient;
    std::unique_ptr<VirtualSceneService::Stub> mService;
};

}  // namespace control
}  // namespace emulation
}  // namespace android