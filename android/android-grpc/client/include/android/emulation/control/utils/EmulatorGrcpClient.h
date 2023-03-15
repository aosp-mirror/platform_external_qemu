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

#include <memory>       // for unique_ptr
#include <optional>     // for optional
#include <string>       // for string
#include <string_view>  // for string_view

#include "aemu/base/logging/CLog.h"
#include "grpc_endpoint_description.pb.h"

namespace android {
namespace emulation {
namespace control {

using ::android::emulation::remote::Endpoint;

// An EmulatorGrpcClient manages the configuration to the emulator grpc
// endpoint. Through this method you can get a properly configured stub, and
// context , that will inject proper credentials when needed.
//
// The Client is initialized by giving it the proper emulator discovery file, or
// by providing it a set of SSL credentials if you wish to use tls.
class EmulatorGrpcClient {
public:
    explicit EmulatorGrpcClient(const std::string_view discovery_file)
        : mDiscoveryFile(discovery_file){};
    EmulatorGrpcClient(const std::string_view address,
                       const std::string_view ca,
                       const std::string_view key,
                       const std::string_view cer);
    EmulatorGrpcClient(const Endpoint& dest);
    ~EmulatorGrpcClient() = default;

    template <class T>
    auto stub() {
        if (!hasOpenChannel()) {
            dwarning(
                    "A gRPC channel to %s discovered by %s is not (yet?) open.",
                    mAddress.c_str(), mDiscoveryFile.c_str());
        }

        return T::NewStub(mChannel);
    }

    std::unique_ptr<grpc::ClientContext> newContext();
    bool hasOpenChannel(bool tryConnect = true);
    std::string address() const { return mAddress; }

    static std::unique_ptr<EmulatorGrpcClient> loadFromProto(
            std::string_view patToEndpointProto);

protected:
    bool initializeChannel();

    std::shared_ptr<::grpc::Channel> mChannel;
    std::shared_ptr<grpc::CallCredentials> mCredentials;
    std::string mDiscoveryFile;
    std::string mAddress;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
