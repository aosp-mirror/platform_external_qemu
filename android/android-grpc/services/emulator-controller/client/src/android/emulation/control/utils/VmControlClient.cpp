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
#include "android/emulation/control/utils/VmControlClient.h"
#include <functional>
#include <memory>
#include <string>
#include <type_traits>

#include <grpcpp/grpcpp.h>
#include <tuple>
#include "android/emulation/control/utils/EmulatorGrcpClient.h"
#include "android/grpc/utils/SimpleAsyncGrpc.h"
#include "emulator_controller.grpc.pb.h"
#include "emulator_controller.pb.h"

namespace android {
namespace emulation {
namespace control {

using ::google::protobuf::Empty;
using ::grpc::Status;

bool VmControlClient::start() {
    VmRunState state;
    state.set_state(VmRunState::START);
    auto context = mClient->newContext();
    Empty response;
    return mService->setVmState(context.get(), state, &response).ok();
}

bool VmControlClient::stop() {
    VmRunState state;
    state.set_state(VmRunState::STOP);
    auto context = mClient->newContext();
    Empty response;
    return mService->setVmState(context.get(), state, &response).ok();
}

void VmControlClient::reset() {
    VmRunState state;
    state.set_state(VmRunState::RESET);
    auto context = mClient->newContext();
    Empty response;
    mService->setVmState(context.get(), state, &response);
}

void VmControlClient::shutdown() {
    VmRunState state;
    state.set_state(VmRunState::SHUTDOWN);
    auto context = mClient->newContext();
    Empty response;
    mService->setVmState(context.get(), state, &response);
}


bool VmControlClient::pause() {
    VmRunState state;
    state.set_state(VmRunState::PAUSED);
    auto context = mClient->newContext();
    Empty response;
    return mService->setVmState(context.get(), state, &response).ok();
}

bool VmControlClient::resume() {
    VmRunState state;
    state.set_state(VmRunState::START);
    auto context = mClient->newContext();
    Empty response;
    return mService->setVmState(context.get(), state, &response).ok();
}

bool VmControlClient::isRunning() {
    VmRunState response;
    auto context = mClient->newContext();
    Empty request;
    mService->getVmState(context.get(), request, &response);
    return response.state() == VmRunState::RUNNING;
}

}  // namespace control
}  // namespace emulation
}  // namespace android
