// Copyright (C) 2018 The Android Open Source Project
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
#include <chrono>                                // for seconds
#include <memory>                                // for shared_ptr, unique_ptr
#include <string>                                // for string
#include <vector>                                // for vector

#include "android/console.h"                     // for AndroidConsoleAgents
#include "grpcpp/security/server_credentials.h"  // for ServerCredentials
#include "grpcpp/impl/codegen/service_type.h"    // for Service

#ifdef _MSC_VER
#include "msvc-posix.h"
#endif

namespace android {
namespace emulation {
namespace control {

// Controller class for the gRPC endpoint, can be used to stop the service or
// inspect some of its properties.
class EmulatorControllerService {
public:
    class Builder;

    virtual ~EmulatorControllerService() {}

    // Stops the gRPC service, this will gracefully close all the
    // incoming/outgoing connections. Closing of connections is not
    // instanteanous and can take up to a second.
    virtual void stop() = 0;

    // The port that the gRPC service is available on.
    virtual int port() = 0;

    // The location of the public certificate that can be used by clients to
    // authenticate to the service.
    virtual std::string publicCert() = 0;
};

// A Factory class that is capable of constructing a proper gRPC service that
// can be used to control the emulator
class EmulatorControllerService::Builder {
public:
    Builder();

    // Sets the console agents to be used. Construction will fail without
    // setting the console agents.
    Builder& withConsoleAgents(const AndroidConsoleAgents* const consoleAgents);

    // The certificate chain and private key that should be used. Setting a
    // certificate chain and private key will enable TLS, not calling this will
    // start the service in an unsecure fashion.
    Builder& withCertAndKey(std::string certfile, std::string privateKeyFile);

    // Enables the gRPC service that binds on the given address on the first
    // port available in the port range [startPart, endPort).
    //
    // - |start| The beginning port range
    // - |end| The last range in the port. The service will NOT bind to this
    // port, should be > start.
    Builder& withPortRange(int start, int end);

    // The IP address to bind to. For example "127.0.0.1" to bind to the
    // loopback device or "0.0.0.0" to bind to all available devices.
    Builder& withAddress(std::string address);

    Builder& withService(::grpc::Service* service);

    // Shutdown the emulator after timeout seconds of gRPC inactivity.
    // The timeout should be at least 1 second, otherwise it will
    // be ignored.
    Builder& withIdleTimeout(std::chrono::seconds timeout);

    // Returns the fully configured and running service, or nullptr if
    // construction failed.
    std::unique_ptr<EmulatorControllerService> build();

private:
    int port();
    const AndroidConsoleAgents* mAgents;
    int mPort{-1};
    std::chrono::seconds mTimeout{0};
    std::vector<std::shared_ptr<::grpc::Service>> mServices;
    std::shared_ptr<grpc::ServerCredentials> mCredentials;
    std::string mBindAddress{"127.0.0.1"};
    std::string mCertfile;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
