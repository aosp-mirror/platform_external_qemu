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

#include "android/emulation/control/GrpcServices.h"

#ifdef _MSC_VER
#include "msvc-posix.h"
#endif
#include <assert.h>
#include <grpcpp/grpcpp.h>
#include <stdio.h>

#include <chrono>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "android/base/Log.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/system/System.h"
#include "android/console.h"
#include "android/emulation/control/RtcBridge.h"
#include "android/emulation/control/interceptor/IdleInterceptor.h"
#include "android/emulation/control/interceptor/LoggingInterceptor.h"
#include "android/emulation/control/interceptor/MetricsInterceptor.h"
#include "grpcpp/server_builder_impl.h"
#include "grpcpp/server_impl.h"

namespace android {
namespace emulation {
namespace control {

using Builder = EmulatorControllerService::Builder;
using namespace android::base;
using namespace android::control::interceptor;
using grpc::ServerBuilder;
using grpc::Service;

// This class owns all the created resources, and is repsonsible for stopping
// and properly releasing resources.
class EmulatorControllerServiceImpl : public EmulatorControllerService {
public:
    void stop() override {
        auto deadline = std::chrono::system_clock::now() +
                        std::chrono::milliseconds(500);
        mServer->Shutdown(deadline);
    }

    EmulatorControllerServiceImpl(
            int port,
            std::vector<std::shared_ptr<Service>> services,
            grpc::Server* server)
        : mPort(port),
          mRegisteredServices(services),
          mServer(server) {}

    int port() override { return mPort; }

private:
    std::unique_ptr<grpc::Server> mServer;
    std::vector<std::shared_ptr<Service>> mRegisteredServices;
    int mPort;
    std::string mCert;
};

Builder::Builder() : mCredentials{grpc::InsecureServerCredentials()} {}

Builder& Builder::withConsoleAgents(
        const AndroidConsoleAgents* const consoleAgents) {
    mAgents = consoleAgents;
    return *this;
}

int Builder::port() {
    return mPort;
}

Builder& Builder::withService(Service* service) {
    if (service != nullptr)
        mServices.emplace_back(std::shared_ptr<Service>(service));
    return *this;
}

Builder& Builder::withCertAndKey(const char* certfile,
                                 const char* privateKeyFile,
                                 const char* caFile) {
    if (!certfile) {
        LOG(WARNING) << "No certfile, security disabled";
        return *this;
    }

    if (!privateKeyFile) {
        LOG(WARNING) << "No private key, security disabled";
        return *this;
    }

    mCertfile = certfile;
    std::ifstream key_file(privateKeyFile);
    std::string key((std::istreambuf_iterator<char>(key_file)),
                    std::istreambuf_iterator<char>());

    std::ifstream cert_file(certfile);
    std::string cert((std::istreambuf_iterator<char>(cert_file)),
                     std::istreambuf_iterator<char>());

    grpc::SslServerCredentialsOptions::PemKeyCertPair keycert = {key, cert};
    grpc::SslServerCredentialsOptions ssl_opts;
    ssl_opts.pem_key_cert_pairs.push_back(keycert);

    // Register the certificate authority if one exists.
    if (caFile) {
        std::ifstream ca_file(caFile);
        std::string ca((std::istreambuf_iterator<char>(ca_file)),
                       std::istreambuf_iterator<char>());
        ssl_opts.pem_root_certs = ca;
        ssl_opts.client_certificate_request =
                GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY;
    }

    mCredentials = grpc::SslServerCredentials(ssl_opts);
    return *this;
}

Builder& Builder::withAddress(std::string address) {
    mBindAddress = address;
    return *this;
}

Builder& Builder::withIdleTimeout(std::chrono::seconds timeout) {
    mTimeout = timeout;
    return *this;
}

Builder& Builder::withPortRange(int start, int end) {
    assert(end > start);
    int port = start;
    bool found = false;
    for (port = start; !found && port < end; port++) {
        // Find a free port.
        android::base::ScopedSocket s0(socketTcp4LoopbackServer(port));
        if (s0.valid()) {
            mPort = android::base::socketGetPort(s0.get());
            found = true;
        }
    }

    return *this;
}  // namespace control

std::unique_ptr<EmulatorControllerService> Builder::build() {
    if (mPort == -1) {
        // No agents, or no port was found.
        LOG(INFO) << "No agents, or valid port was found";
        return nullptr;
    }

    std::string server_address = mBindAddress + ":" + std::to_string(mPort);
    ServerBuilder builder;
    builder.AddListeningPort(server_address, mCredentials);
    for (auto service : mServices) {
        builder.RegisterService(service.get());
    }

    // Register logging & metrics interceptor.
    std::vector<std::unique_ptr<
            grpc::experimental::ServerInterceptorFactoryInterface>>
            creators;
    creators.emplace_back(std::make_unique<StdOutLoggingInterceptorFactory>());
    creators.emplace_back(std::make_unique<MetricsInterceptorFactory>());

    if (mTimeout.count() > 0 && mAgents != nullptr) {
        creators.emplace_back(
                std::make_unique<IdleInterceptorFactory>(mTimeout, mAgents));
    }

    builder.experimental().SetInterceptorCreators(std::move(creators));

    auto service = builder.BuildAndStart();
    if (!service)
        return nullptr;

    fprintf(stderr, "Started GRPC server at %s\n", server_address.c_str());
    return std::unique_ptr<EmulatorControllerService>(
            new EmulatorControllerServiceImpl(
                    mPort, std::move(mServices), service.release()));
}
}  // namespace control
}  // namespace emulation
}  // namespace android
