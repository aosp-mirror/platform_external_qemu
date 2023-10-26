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
 * @brief A client for interacting with the Virtual Scene Service.
 *
 * The Virtual Scene Service enables clients to manipulate the virtual scene by
 * setting posters, managing animation states, and retrieving information about
 * the scene. This class provides methods for asynchronously invoking these
 * operations through gRPC.
 */
class VirtualSceneServiceClient {
public:
    /**
     * @brief Constructs a VirtualSceneServiceClient.
     *
     * @param client A shared pointer to the EmulatorGrpcClient used for
     * communication.
     * @param service A pointer to the VirtualSceneService::StubInterface, if
     * available.
     */
    explicit VirtualSceneServiceClient(
            std::shared_ptr<EmulatorGrpcClient> client,
            VirtualSceneService::StubInterface* service = nullptr)
        : mClient(client), mService(service) {
        if (!service) {
            mService = client->stub<VirtualSceneService>();
        }
    }

    /**
     * @brief Asynchronously retrieves the list of posters in the virtual scene.
     *
     * @param onDone The callback to be invoked when the operation completes,
     *              carrying the list of posters.
     */
    void listPostersAsync(OnCompleted<PosterList> onDone);

    /**
     * @brief Asynchronously sets a poster in the virtual scene.
     *
     * @param poster The details of the poster to be set.
     * @param onDone The callback to be invoked when the operation completes,
     *              carrying the updated poster details.
     */
    void setPosterAsync(Poster poster, OnCompleted<Poster> onDone);

    /**
     * @brief Asynchronously sets the animation state of the virtual scene.
     *
     * @param state The new animation state to be set.
     * @param onDone The callback to be invoked when the operation completes,
     *              carrying the updated animation state.
     */
    void setAnimationStateAsync(AnimationState state,
                                OnCompleted<AnimationState> onDone);

    /**
     * @brief Asynchronously retrieves the current animation state of the
     * virtual scene.
     *
     * @param onDone The callback to be invoked when the operation completes,
     *              carrying the current animation state.
     */
    void getAnimationStateAsync(OnCompleted<AnimationState> onDone);

private:
    std::shared_ptr<EmulatorGrpcClient> mClient;
    std::unique_ptr<VirtualSceneService::StubInterface> mService;
};

}  // namespace control
}  // namespace emulation
}  // namespace android