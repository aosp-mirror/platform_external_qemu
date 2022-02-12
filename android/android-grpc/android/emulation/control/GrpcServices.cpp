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
#include <grpcpp/security/server_credentials_impl.h>
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
#include "android/console.h"
#include "android/emulation/control/interceptor/IdleInterceptor.h"
#include "android/emulation/control/interceptor/LoggingInterceptor.h"
#include "android/emulation/control/interceptor/MetricsInterceptor.h"
#include "android/emulation/control/secure/BasicTokenAuth.h"
#include "android/emulation/control/secure/JwtTokenAuth.h"
#include "android/emulation/control/utils/GrpcAndroidLogAdapter.h"
#include "grpc/grpc_security_constants.h"
#include "grpcpp/server_builder_impl.h"
#include "grpcpp/server_impl.h"

namespace android {
namespace emulation {
namespace control {

using Builder = EmulatorControllerService::Builder;
using namespace android::base;
using namespace android::control::interceptor;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::Service;

std::ostream& operator<<(
        std::ostream& os,
        const EmulatorControllerService::Builder::Authorization& a) {
    if (a == EmulatorControllerService::Builder::Authorization::None) {
        os << "none";
    } else if ((int)a & (int)EmulatorControllerService::Builder::Authorization::
                                StaticToken) {
        os << "+token";
    } else if ((int)a & (int)EmulatorControllerService::Builder::Authorization::
                                JwtToken) {
        os << "+jwt";
    }
    return os;
}

// This class owns all the created resources, and is responsible for stopping
// and properly releasing resources.
class EmulatorControllerServiceImpl : public EmulatorControllerService {
public:
    ~EmulatorControllerServiceImpl() { stop(); }

    void stop() override {
        auto deadline = std::chrono::system_clock::now() +
                        std::chrono::milliseconds(500);
        mServer->Shutdown(deadline);
    }

    EmulatorControllerServiceImpl(
            int port,
            std::vector<std::shared_ptr<Service>> services,
            grpc::Server* server,
            std::vector<std::unique_ptr<ServerCompletionQueue>> queue)
        : mPort(port), mRegisteredServices(services), mServer(server) {
        mHandler = std::make_unique<AsyncGrpcHandler>(std::move(queue));
    }

    int port() override { return mPort; }

    void wait() override { mServer->Wait(); }

    // Returns a new completionQueue if possible.
    AsyncGrpcHandler* asyncHandler() override { return mHandler.get(); }

private:
    std::unique_ptr<grpc::Server> mServer;
    std::vector<std::shared_ptr<Service>> mRegisteredServices;
    std::unique_ptr<AsyncGrpcHandler> mHandler;
    int mPort;
    int queueidx = 0;
    std::string mCert;
};

// Returns the whole file contents, or empty if the file could not be read
// or is empty. Will set the valid flag to false if the file cannot be read
// or is empty.
std::string Builder::readSecrets(const char* fname) {
    if (!fname) {
        LOG(ERROR) << "Cannot read secrets from nothing.";
        mValid = false;
        return "";
    }
    if (!System::get()->pathExists(fname)) {
        LOG(ERROR) << "File " << fname << " does not exist or is unreadable";
        mValid = false;
        return "";
    }
    std::ifstream fstream(fname);
    auto contents = std::string(std::istreambuf_iterator<char>(fstream),
                                std::istreambuf_iterator<char>());

    if (fstream.fail()) {
        LOG(ERROR) << "Failure while reading from: " << fname;
        mValid = false;
    } else if (contents.empty()) {
        LOG(ERROR) << "The file " << fname << " is empty.";
        mValid = true;
    }

    return contents;
}

Builder::Builder() = default;

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

Builder& Builder::withSecureService(Service* service) {
    if (service != nullptr)
        mSecureServices.emplace_back(std::shared_ptr<Service>(service));
    return *this;
}

Builder& Builder::withAuthToken(std::string token) {
    mAuthToken = token;
    mValid = !token.empty();
    mAuthMode = mAuthMode | Authorization::StaticToken;
    return *this;
}

Builder& Builder::withJwtAuthDiscoveryDir(std::string jwks,
                                          std::string jwkLoadedPath) {
    mJwkPath = jwks;
    mJwkLoadedPath = jwkLoadedPath;
    mValid =
            System::get()->pathExists(jwks) && System::get()->pathCanRead(jwks);
    mAuthMode = mAuthMode | Authorization::JwtToken;
    return *this;
}

Builder& Builder::withCertAndKey(const char* certfile,
                                 const char* privateKeyFile,
                                 const char* caFile) {
    if (!certfile) {
        return *this;
    }

    if (!privateKeyFile) {
        return *this;
    }

    mCertfile = certfile;
    auto key = readSecrets(privateKeyFile);
    auto cert = readSecrets(certfile);

    grpc::SslServerCredentialsOptions::PemKeyCertPair keycert = {key, cert};
    grpc::SslServerCredentialsOptions ssl_opts;
    ssl_opts.pem_key_cert_pairs.push_back(keycert);

    // Register the certificate authority if one exists.
    if (caFile) {
        auto ca = readSecrets(caFile);
        ssl_opts.pem_root_certs = ca;
        ssl_opts.client_certificate_request =
                GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY;
        mCaCerts = true;
    }

    mCredentials = grpc::SslServerCredentials(ssl_opts);
    mSecurity = Security::Tls;
    return *this;
}

Builder& Builder::withVerboseLogging(bool verbose) {
    mVerbose = verbose;
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

Builder& Builder::withLogging(bool logging) {
    mLogging = logging;
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
            mIpMode = IpMode::Ipv4;
            found = true;
        } else {
            // Try ipv6 port
            s0 = socketTcp6LoopbackServer(port);
            if (s0.valid()) {
                mPort = android::base::socketGetPort(s0.get());
                mIpMode = IpMode::Ipv6;
                found = true;
            }
        }
    }
    return *this;
}

Builder& Builder::withAsyncServerThreads(int count) {
    mCompletionQueueCount = count;
    return *this;
}

//  Human readable logging.
template <typename tstream>
tstream& operator<<(tstream& out, const Builder::Security value) {
    const char* s = 0;
#define STATE(p)                 \
    case (Builder::Security::p): \
        s = #p;                  \
        break;
    switch (value) {
        STATE(Insecure);
        STATE(Tls);
        STATE(Local)
    }
#undef STATE
    return out << s;
}

std::unique_ptr<EmulatorControllerService> Builder::build() {
    // Setup a log redirector.
    if (mVerbose) {
        gpr_set_log_function(&gpr_log_to_android_log);
    } else {
        gpr_set_log_function(&gpr_null_logger);
    }

    if (mPort == -1) {
        // No agents, or no port was found.
        LOG(INFO) << "No agents, or valid port was found";
        return nullptr;
    }

    if (!mValid) {
        LOG(ERROR) << "Couldn't configure security system.";
        return nullptr;
    }

    if (!mCredentials) {
        if (mBindAddress == "localhost" || mBindAddress == "[::1]") {
            mCredentials = LocalServerCredentials(LOCAL_TCP);
            mSecurity = Security::Local;
        } else {
            mCredentials = grpc::InsecureServerCredentials();
            mSecurity = Security::Insecure;
        }
    }

    if (!mAuthToken.empty() || !mJwkPath.empty()) {
        if (mSecurity == Security::Insecure) {
            mBindAddress = "[::1]";
            mCredentials = LocalServerCredentials(LOCAL_TCP);
            mSecurity = Security::Local;
            LOG(WARNING) << "Token/JWT auth requested without tls, restricting "
                            "access to localhost.";
        }

        std::vector<BasicTokenAuth> protectors;
        if (!mAuthToken.empty() && !mJwkPath.empty()) {
            auto anyauth = std::vector<std::unique_ptr<BasicTokenAuth>>();
            anyauth.emplace_back(std::make_unique<StaticTokenAuth>(mAuthToken));
            anyauth.emplace_back(
                    std::make_unique<JwtTokenAuth>(mJwkPath, mJwkLoadedPath));
            mCredentials->SetAuthMetadataProcessor(
                    std::make_shared<AnyTokenAuth>(std::move(anyauth)));
        } else if (!mAuthToken.empty()) {
            mCredentials->SetAuthMetadataProcessor(
                    std::make_shared<StaticTokenAuth>(mAuthToken));
        } else if (!mJwkPath.empty()) {
            mCredentials->SetAuthMetadataProcessor(
                    std::make_shared<JwtTokenAuth>(mJwkPath, mJwkLoadedPath));
        }
    } else {
        dwarning(
                "*** No gRPC protection active, consider launching with the "
                "-grpc-use-jwt flag.***");
    }
    // Translate loopback Ipv4/Ipv6 preference ourselves. gRPC resolver can
    // do it slightly differently than us, leading to unexpected results.
    if (mBindAddress == "[::1]" || mBindAddress == "127.0.0.1" ||
        mBindAddress == "localhost") {
        mBindAddress = (mIpMode == IpMode::Ipv4 ? "127.0.0.1" : "[::1]");
    }

    std::string server_address = mBindAddress + ":" + std::to_string(mPort);

    ServerBuilder builder;
    builder.AddListeningPort(server_address, mCredentials);
    for (auto service : mServices) {
        builder.RegisterService(service.get());
    }

    if (mSecurity == Security::Tls && mCaCerts) {
        for (auto service : mSecureServices) {
            builder.RegisterService(service.get());
        }
    }
    // Register logging & metrics interceptor.
    std::vector<std::unique_ptr<
            grpc::experimental::ServerInterceptorFactoryInterface>>
            creators;

    if (mLogging) {
        creators.emplace_back(
                std::make_unique<StdOutLoggingInterceptorFactory>());
    }
    creators.emplace_back(std::make_unique<MetricsInterceptorFactory>());
    if (mTimeout.count() > 0 && mAgents != nullptr) {
        creators.emplace_back(
                std::make_unique<IdleInterceptorFactory>(mTimeout, mAgents));
    }
    builder.experimental().SetInterceptorCreators(std::move(creators));

    // Create a separate cq for each potential handler.
    std::vector<std::unique_ptr<ServerCompletionQueue>> cqs;
    for (int i = 0; i < mCompletionQueueCount; i++) {
        cqs.push_back(builder.AddCompletionQueue(false));
    }

    auto service = builder.BuildAndStart();
    if (!service)
        return nullptr;

    LOG(INFO) << "Started GRPC server at " << server_address.c_str()
              << ", security: " << mSecurity << ":" << mAuthMode;
    return std::unique_ptr<EmulatorControllerService>(
            new EmulatorControllerServiceImpl(mPort, std::move(mServices),
                                              service.release(),
                                              std::move(cqs)));
}
}  // namespace control
}  // namespace emulation
}  // namespace android
