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

#include <grpcpp/grpcpp.h>  // for ClientContext

#include <memory>  // for unique_ptr
#include <string>  // for string

#include "emulator_controller.grpc.pb.h"  // for EmulatorController

namespace emulator {
namespace webrtc {

class EmulatorGrpcClient {
public:
    explicit EmulatorGrpcClient(std::string discovery_file) : mDiscoveryFile(discovery_file) {};
    android::emulation::control::EmulatorController::Stub* stub();

private:
    bool initializeGrpcStub();

    std::unique_ptr<android::emulation::control::EmulatorController::Stub>
            mEmulatorStub;
    grpc::ClientContext mContext;
    std::string mDiscoveryFile;
};

}  // namespace webrtc
}  // namespace emulator
