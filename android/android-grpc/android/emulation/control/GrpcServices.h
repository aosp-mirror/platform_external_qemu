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
#include <chrono>  // for seconds
#include <memory>  // for shared_ptr, unique_ptr
#include <string>  // for string
#include <vector>  // for vector

#include <grpcpp/grpcpp.h>
#include "android/console.h"                     // for AndroidConsoleAgents
#include "grpcpp/impl/codegen/service_type.h"    // IWYU pragma: keep
#include "grpcpp/security/server_credentials.h"  // for ServerCredentials
#include "android/emulation/control/async/AsyncGrcpStream.h"

#ifdef _MSC_VER
#include "msvc-posix.h"
#endif

namespace android {
namespace emulation {
namespace control {

using grpc::ServerCompletionQueue;

// Controller class for the gRPC endpoint, can be used to stop the service or
// inspect some of its properties.
class EmulatorControllerService {
public:
    class Builder;

    virtual ~EmulatorControllerService() = default;

    // Stops the gRPC service, this will gracefully close all the
    // incoming/outgoing connections. Closing of connections is not
    // instanteanous and can take up to a second.
    virtual void stop() = 0;

    // The port that the gRPC service is available on.
    virtual int port() = 0;

    // Block and wait until the server is completed.
    virtual void wait() = 0;

    // You can register your asynchronous bidi connections on this.
    virtual AsyncGrpcHandler* asyncHandler() = 0;
};

// A Factory class that is capable of constructing a proper gRPC service that
// can be used to control the emulator
class EmulatorControllerService::Builder {
public:
    enum class Security { Insecure = 0, Tls, Local };
    enum class Authorization { None = 0, StaticToken = 1, JwtToken = 2 };
    enum class IpMode { Ipv4, Ipv6 };

    Builder();

    // Sets the console agents to be used. Construction will fail without
    // setting the console agents.
    Builder& withConsoleAgents(const AndroidConsoleAgents* const consoleAgents);

    // The certificate chain and private key that should be used. Setting a
    // certificate chain and private key will enable TLS, not calling this will
    // start the service in an unsecure fashion.
    Builder& withCertAndKey(const char* certfile,
                            const char* privateKeyFile,
                            const char* certAuthority);

    // Reject any request with the status UNAUTHORIZED if the following header
    // is not present: Authorization: Bearer <token>
    //
    // |token| Token to use.
    Builder& withAuthToken(std::string token);

    // Reject any request with the status UNAUTHORIZED if the following header
    // is not present: Authorization: Bearer <token>
    //
    // If the header is present it must contain a valid JWT token with:
    //
    // aud: Set to the path of the gRPC request, and it must be signed
    // with a key that can be found in the jwk Directory.
    //
    // |jwks| Directory where .jwk files can be written that contain
    // valid JSON web keys used to validate the JWT.
    // |jwkLoadedPath| The path to a file where the emulator will write the
    // set of loaded public web keys.
    Builder& withJwtAuthDiscoveryDir(std::string jwks, std::string jwkLoadedPath);

    // True to enable verbose logging.
    // Verbose logging will connect the gRPC logging engine to the
    // android emulator log engine. Setting this to false will
    // disable all logging.
    Builder& withVerboseLogging(bool verbose);

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

    // Add a service only if tls and client-ca is enabled.
    Builder& withSecureService(::grpc::Service* service);

    // Register count number of threads to handle async
    // requests.
    //
    Builder& withAsyncServerThreads(int count);

    // Shutdown the emulator after timeout seconds of gRPC inactivity.
    // The timeout should be at least 1 second, otherwise it will
    // be ignored.
    Builder& withIdleTimeout(std::chrono::seconds timeout);

    // Returns the fully configured and running service, or nullptr if
    // construction failed.
    std::unique_ptr<EmulatorControllerService> build();

    Builder& withLogging(bool logging);

private:
    std::string readSecrets(const char* fname);

    int port();
    const AndroidConsoleAgents* mAgents;
    int mPort{-1};
    std::chrono::seconds mTimeout{0};
    std::vector<std::shared_ptr<::grpc::Service>> mServices;
    std::vector<std::shared_ptr<::grpc::Service>> mSecureServices;
    Security mSecurity{Security::Insecure};
    std::shared_ptr<grpc::ServerCredentials> mCredentials;
    std::string mBindAddress{"127.0.0.1"};
    std::string mCertfile;
    std::string mAuthToken;
    std::string mJwkPath;
    std::string mJwkLoadedPath;
    Authorization mAuthMode{Authorization::None};
    bool mValid{true};
    bool mVerbose{false};
    bool mLogging{true};
    bool mCaCerts{false};
    int mCompletionQueueCount{0};
    IpMode mIpMode{IpMode::Ipv4};
};

}  // namespace control
}  // namespace emulation
}  // namespace android
