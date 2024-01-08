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

#include <grpcpp/grpcpp.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "absl/status/statusor.h"
#include "aemu/base/logging/Log.h"
#include "android/emulation/control/utils/Library.h"
#include "grpc_endpoint_description.pb.h"

namespace android {
namespace emulation {
namespace control {

using ::android::emulation::remote::Endpoint;
using grpc::experimental::ClientInterceptorFactoryInterface;
using InterceptorFactory = std::unique_ptr<ClientInterceptorFactoryInterface>;
using InterceptorFactories = std::vector<InterceptorFactory>;

// An EmulatorGrpcClient manages the configuration to the emulator grpc
// endpoint. Through this method you can get a properly configured stub, and
// context , that will inject proper credentials when needed.
//
// The Client is initialized by giving it the proper emulator discovery file, or
// by providing it a set of SSL credentials if you wish to use tls.
class EmulatorGrpcClient {
public:
    class Builder;
    friend class Builder;

    virtual ~EmulatorGrpcClient() = default;

    template <class T>
    auto stub() {
        if (!hasOpenChannel()) {
            dwarning("A gRPC channel to %s is not (yet?) open.",
                     address().c_str());
        }

        return T::NewStub(mChannel);
    }

    // A client context will be tracked, so it is possible to cancel
    // all active connections made from this client.
    std::shared_ptr<grpc::ClientContext> newContext();

    // This will call TryCancel on all activeContexts.
    void cancelAll(std::chrono::milliseconds maxWait = std::chrono::seconds(1));
    virtual bool hasOpenChannel(bool tryConnect = true);
    std::string address() const { return mEndpoint.target(); }

    static absl::StatusOr<std::unique_ptr<EmulatorGrpcClient>> loadFromProto(
            std::string_view patToEndpointProto,
            InterceptorFactories factory = {});

    // Returns a connection to the current emulator
    static std::shared_ptr<EmulatorGrpcClient> me();

    // Configure the "me" singleton based upon the endpoint definition
    static void configureMe(std::unique_ptr<EmulatorGrpcClient> me);

protected:
    EmulatorGrpcClient() = default;
    EmulatorGrpcClient(Endpoint dest, InterceptorFactories factories);

private:
    Endpoint mEndpoint;
    Library<::grpc::ClientContext> mActiveContexts;
    std::shared_ptr<::grpc::Channel> mChannel;
    std::shared_ptr<grpc::CallCredentials> mCredentials;

};

class EmulatorTestClient : public EmulatorGrpcClient {
public:
    EmulatorTestClient() {}
    bool hasOpenChannel(bool tryConnect = true) override { return true; }
};

class EmulatorGrpcClient::Builder {
public:
    Builder& withInterceptor(ClientInterceptorFactoryInterface* factory);
    Builder& withInterceptors(InterceptorFactories factories);
    Builder& withDiscoveryFile(std::string discovery_file);
    Builder& withEndpoint(const Endpoint& destination);
    absl::StatusOr<std::unique_ptr<EmulatorGrpcClient>> build();

private:
    absl::Status mStatus{absl::OkStatus()};
    InterceptorFactories mFactories;
    Endpoint mDestination;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
