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
#include "android/emulation/control/utils/EmulatorGrcpClient.h"

#include <grpcpp/grpcpp.h>  // for Clie...
#include <fstream>          // for basi...
#include <iostream>         // for ios::
#include <map>              // for mult...
#include <memory>           // for shar......
#include <string>           // for string
#include <type_traits>      // for remo...
#include <unordered_map>    // for __ha...
#include <utility>          // for move

#include "aemu/base/Stopwatch.h"  // for Stop...

#include "aemu/base/files/IniFile.h"    // for IniFile
#include "aemu/base/files/PathUtils.h"  // for Path...
#include "aemu/base/logging/CLog.h"
#include "android/emulation/control/EmulatorAdvertisement.h"
#include "android/emulation/control/interceptor/LoggingInterceptor.h"
#include "android/emulation/control/interceptor/MetricsInterceptor.h"
#include "android/emulation/control/secure/BasicTokenAuth.h"  // for Basi...
#include "grpc/grpc_security_constants.h"                     // for LOCA...
#include "grpc/impl/codegen/connectivity_state.h"             // for GRPC...
#include "grpc/impl/codegen/gpr_types.h"                      // for gpr_...
#include "grpc/support/time.h"                                // for gpr_now
#include "grpc_endpoint_description.pb.h"                     // for Endp...
#include "grpcpp/security/credentials.h"                      // for Meta...
#include "grpcpp/support/channel_arguments.h"                 // for Chan...

#define DEBUG 0
/* set  >1 for very verbose debugging */
#if DEBUG <= 1
#define DD(...) (void)0
#else
#define DD(...) dinfo(__VA_ARGS__)
#endif

using android::control::interceptor::MetricsInterceptorFactory;
using android::control::interceptor::StdOutLoggingInterceptorFactory;
namespace android {
namespace emulation {
namespace control {
using android::base::PathUtils;
using android::base::Stopwatch;

static bool startsWith(const std::string& str, const std::string& prefix) {
    return str.compare(0, prefix.size(), prefix) == 0;
}

static bool isLocalhost(const std::string& ipAddress) {
    return startsWith(ipAddress, "127.0.0.1") || startsWith(ipAddress, "::1") ||
           startsWith(ipAddress, "localhost");
}

// A plugin that inserts the basic auth headers into a gRPC request
// if needed.
class BasicTokenAuthenticator : public grpc::MetadataCredentialsPlugin {
public:
    BasicTokenAuthenticator(const grpc::string& token)
        : mToken("Bearer " + token) {}
    ~BasicTokenAuthenticator() = default;

    grpc::Status GetMetadata(
            grpc::string_ref service_url,
            grpc::string_ref method_name,
            const grpc::AuthContext& channel_auth_context,
            std::multimap<grpc::string, grpc::string>* metadata) override {
        metadata->insert(std::make_pair(
                android::emulation::control::BasicTokenAuth::DEFAULT_HEADER,
                mToken));
        return grpc::Status::OK;
    }

private:
    grpc::string mToken;
};

using HeaderMap = std::unordered_map<std::string, std::string>;

// A plugin that inserts a set of headers.
class HeaderInjector : public grpc::MetadataCredentialsPlugin {
public:
    HeaderInjector(HeaderMap&& headers) : mHeaders(headers) {}
    ~HeaderInjector() = default;

    grpc::Status GetMetadata(
            grpc::string_ref service_url,
            grpc::string_ref method_name,
            const grpc::AuthContext& channel_auth_context,
            std::multimap<grpc::string, grpc::string>* metadata) override {
        for (const auto& v : mHeaders) {
            DD("Header key: %s", v.first.c_str());
            DD("Header val: %s", v.second.c_str());
            metadata->insert(std::make_pair(v.first, v.second));
        }
        return grpc::Status::OK;
    }

private:
    HeaderMap mHeaders;
};

bool EmulatorGrpcClient::hasOpenChannel(bool tryConnect) {
    if (!mChannel) {
        initializeChannel();
    }
    if (!mChannel)
        return false;

    if (tryConnect) {
        Stopwatch sw;
        auto waitUntil = gpr_time_add(gpr_now(GPR_CLOCK_REALTIME),
                                      gpr_time_from_seconds(5, GPR_TIMESPAN));
        bool connect = mChannel->WaitForConnected(waitUntil);
        double time = Stopwatch::sec(sw.elapsedUs());
        DD("%s to emulator: %s, after %f us",
           connect ? "Connected" : "Not connected", mAddress.c_str(), time);
    }
    auto state = mChannel->GetState(tryConnect);
    return state == GRPC_CHANNEL_READY || state == GRPC_CHANNEL_IDLE;
}

static std::string readFile(std::string_view fname) {
    std::ifstream fstream(PathUtils::asUnicodePath(fname.data()).c_str());
    std::string contents((std::istreambuf_iterator<char>(fstream)),
                         std::istreambuf_iterator<char>());
    return contents;
}

EmulatorGrpcClient::EmulatorGrpcClient(const std::string_view address,
                                       const std::string_view ca,
                                       const std::string_view key,
                                       const std::string_view cer)
    : mAddress(address) {
    DD("Address: %s", mAddress.c_str());
    std::shared_ptr<grpc::ChannelCredentials> call_creds;
    if (isLocalhost(mAddress)) {
        call_creds = ::grpc::experimental::LocalCredentials(LOCAL_TCP);
    } else {
        call_creds = ::grpc::InsecureChannelCredentials();
    }

    if (!ca.empty() && !key.empty() && !cer.empty()) {
        DD("Using tls with");
        DD("ca: %s", ca.data());
        DD("key: %s", key.data());
        DD("cer: %s", cer.data());
        // Client will use the server cert as the ca authority.
        grpc::SslCredentialsOptions sslOpts;
        sslOpts.pem_root_certs = readFile(ca);
        sslOpts.pem_private_key = readFile(key);
        sslOpts.pem_cert_chain = readFile(cer);
        call_creds = grpc::SslCredentials(sslOpts);
    }

    std::vector<std::unique_ptr<
            grpc::experimental::ClientInterceptorFactoryInterface>>
            interceptors;
    if (VERBOSE_CHECK(grpc)) {
        interceptors.push_back(std::unique_ptr<StdOutLoggingInterceptorFactory>(
                new StdOutLoggingInterceptorFactory()));
    }
    interceptors.emplace_back(std::make_unique<MetricsInterceptorFactory>());

    grpc::ChannelArguments maxSize;
    maxSize.SetMaxReceiveMessageSize(-1);
    mChannel = grpc::experimental::CreateCustomChannelWithInterceptors(
            mAddress, call_creds, maxSize, std::move(interceptors));
}

EmulatorGrpcClient::EmulatorGrpcClient(const Endpoint& dest)
    : EmulatorGrpcClient(dest.target(),
                         dest.tls_credentials().pem_root_certs(),
                         dest.tls_credentials().pem_private_key(),
                         dest.tls_credentials().pem_cert_chain()) {
    if (!dest.required_headers().empty()) {
        HeaderMap map;
        for (const auto& header : dest.required_headers()) {
            DD("Adding header: %s, %s", header.key().c_str(),
               header.value().c_str());
            map[header.key()] = header.value();
        }
        mCredentials = grpc::MetadataCredentialsFromPlugin(
                std::make_unique<HeaderInjector>(std::move(map)));
    }
}

std::unique_ptr<grpc::ClientContext> EmulatorGrpcClient::newContext() {
    auto ctx = std::make_unique<grpc::ClientContext>();
    if (mCredentials) {
        ctx->set_credentials(mCredentials);
    }

    return ctx;
}

bool EmulatorGrpcClient::initializeChannel() {
    android::base::IniFile iniFile(mDiscoveryFile);
    iniFile.read();
    if (!iniFile.hasKey("grpc.port") || iniFile.hasKey("grpc.server_cert")) {
        derror("No grpc port, or tls enabled. gRPC disabled.");
        return false;
    }
    mAddress = "localhost:" + iniFile.getString("grpc.port", "8554");

    std::vector<std::unique_ptr<
            grpc::experimental::ClientInterceptorFactoryInterface>>
            interceptors;
    if (VERBOSE_CHECK(grpc)) {
        interceptors.push_back(std::unique_ptr<StdOutLoggingInterceptorFactory>(
                new StdOutLoggingInterceptorFactory()));
    }

    interceptors.emplace_back(std::make_unique<MetricsInterceptorFactory>());
    grpc::ChannelArguments maxSize;
    maxSize.SetMaxReceiveMessageSize(-1);
    mChannel = grpc::experimental::CreateCustomChannelWithInterceptors(
            mAddress, ::grpc::experimental::LocalCredentials(LOCAL_TCP),
            maxSize, std::move(interceptors));

    // Install token authenticator if needed.
    if (iniFile.hasKey("grpc.token")) {
        auto token = iniFile.getString("grpc.token", "");
        mCredentials = grpc::MetadataCredentialsFromPlugin(
                std::make_unique<BasicTokenAuthenticator>(token));
    }
    return true;
}

std::unique_ptr<EmulatorGrpcClient> EmulatorGrpcClient::loadFromProto(
        std::string_view pathToEndpointProto) {
    // Read the existing address book.
    std::fstream input(
            PathUtils::asUnicodePath(pathToEndpointProto.data()).c_str(),
            std::ios::in | std::ios::binary);
    Endpoint endpoint;
    if (!input) {
        derror("File %s not found",
               PathUtils::asUnicodePath(pathToEndpointProto.data()).c_str());
        return nullptr;
    } else if (!endpoint.ParseFromIstream(&input)) {
        derror("File %s does not contain a valid endpoint",
               PathUtils::asUnicodePath(pathToEndpointProto.data()).c_str());
        return nullptr;
    }

    return std::make_unique<EmulatorGrpcClient>(endpoint);
}

std::shared_ptr<EmulatorGrpcClient> sMe;
std::mutex sMeLock;

std::shared_ptr<EmulatorGrpcClient> EmulatorGrpcClient::me() {
    std::lock_guard<std::mutex> guard(sMeLock);
    if (sMe) {
        return sMe;
    }

    auto discovery = EmulatorAdvertisement({}).location();
    sMe = std::make_shared<EmulatorGrpcClient>(discovery);
    return sMe;
}

void EmulatorGrpcClient::configureMe(const remote::Endpoint& endpoint) {
    std::lock_guard<std::mutex> guard(sMeLock);
    sMe = std::make_shared<EmulatorGrpcClient>(endpoint);
}
}  // namespace control
}  // namespace emulation
}  // namespace android
