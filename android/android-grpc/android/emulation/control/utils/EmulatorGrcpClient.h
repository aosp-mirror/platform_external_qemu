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

namespace android {
namespace emulation {
namespace control {

// An EmulatorGrpcClient manages the configuration to the emulator grpc
// endpoint. Through this method you can get a properly configured stub, and
// context , that will inject proper credentials when needed.
//
// The Client is initialized by giving it the proper emulator discovery file, or
// by providing it a set of SSL credentials if you wish to use tls.
class EmulatorGrpcClient {
public:
    explicit EmulatorGrpcClient(std::string discovery_file)
        : mDiscoveryFile(discovery_file){};
    EmulatorGrpcClient(std::string address,
                       std::string ca,
                       std::string key,
                       std::string cer);
    ~EmulatorGrpcClient() = default;

    template <class stub>
    auto stub() {
        if (!mChannel) {
            initializeChannel();
        }

        return stub::NewStub(mChannel);
    }

    std::unique_ptr<grpc::ClientContext> newContext();
    bool hasOpenChannel(bool tryConnect = true);
    std::string address() { return mAddress; }

protected:
    bool initializeChannel();

    std::shared_ptr<::grpc::Channel> mChannel;
    std::shared_ptr<grpc_impl::CallCredentials> mCredentials;
    std::string mDiscoveryFile;
    std::string mAddress;
};

}  // namespace control
}  // namespace emulation
}  // namespace android