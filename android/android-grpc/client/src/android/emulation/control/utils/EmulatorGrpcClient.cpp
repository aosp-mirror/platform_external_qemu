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

#include <grpcpp/grpcpp.h>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "aemu/base/Stopwatch.h"

#include "aemu/base/files/PathUtils.h"
#include "aemu/base/logging/Log.h"
#include "android/base/files/IniFile.h"
#include "android/emulation/control/EmulatorAdvertisement.h"
#include "android/emulation/control/secure/BasicTokenAuth.h"
#include "grpc/grpc_security_constants.h"
#include "grpc/impl/codegen/connectivity_state.h"
#include "grpc/impl/codegen/gpr_types.h"
#include "grpc/support/time.h"
#include "grpc_endpoint_description.pb.h"
#include "grpcpp/security/credentials.h"
#include "grpcpp/support/channel_arguments.h"

#define DEBUG 0
/* set  >1 for very verbose debugging */
#if DEBUG <= 1
#define DD(...) (void)0
#else
#define DD(...) dinfo(__VA_ARGS__)
#endif

namespace android {
namespace emulation {
namespace control {

using Builder = EmulatorGrpcClient::Builder;
using android::base::PathUtils;
using android::base::Stopwatch;
using ::android::emulation::remote::Header;

static bool startsWith(const std::string& str, const std::string& prefix) {
    return str.compare(0, prefix.size(), prefix) == 0;
}

static bool isLocalhost(const std::string& ipAddress) {
    return startsWith(ipAddress, "127.0.0.1") || startsWith(ipAddress, "::1") ||
           startsWith(ipAddress, "localhost");
}

static std::string readFile(std::string_view fname) {
    std::ifstream fstream(PathUtils::asUnicodePath(fname.data()).c_str());
    std::string contents((std::istreambuf_iterator<char>(fstream)),
                         std::istreambuf_iterator<char>());
    return contents;
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

EmulatorGrpcClient::EmulatorGrpcClient(Endpoint dest,
                                       InterceptorFactories interceptors)
    : mEndpoint(dest) {
    auto address = dest.target();
    std::shared_ptr<grpc::ChannelCredentials> call_creds;

    std::string ca = dest.tls_credentials().pem_root_certs();
    std::string key = dest.tls_credentials().pem_private_key();
    std::string cer = dest.tls_credentials().pem_cert_chain();

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
    } else if (isLocalhost(address)) {
        call_creds = ::grpc::experimental::LocalCredentials(LOCAL_TCP);
    } else {
        call_creds = ::grpc::InsecureChannelCredentials();
    }

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
    grpc::ChannelArguments maxSize;
    maxSize.SetMaxReceiveMessageSize(-1);
    mChannel = grpc::experimental::CreateCustomChannelWithInterceptors(
            address, call_creds, maxSize, std::move(interceptors));
}

std::shared_ptr<grpc::ClientContext> EmulatorGrpcClient::newContext() {
    auto ctx = mActiveContexts.acquire();
    if (mCredentials) {
        ctx->set_credentials(mCredentials);
    }

    return ctx;
}

void EmulatorGrpcClient::cancelAll(std::chrono::milliseconds maxWait) {
    mActiveContexts.forEach([](auto context) { context->TryCancel(); });
    mActiveContexts.waitUntilLibraryIsClear(maxWait);
}

absl::StatusOr<std::unique_ptr<EmulatorGrpcClient>>
EmulatorGrpcClient::loadFromProto(std::string_view pathToEndpointProto,
                                  InterceptorFactories factories) {
    // Read the existing address book.
    std::fstream input(
            PathUtils::asUnicodePath(pathToEndpointProto.data()).c_str(),
            std::ios::in | std::ios::binary);
    Endpoint endpoint;
    if (!input) {
        derror("File %s not found", pathToEndpointProto.data());
        return nullptr;
    } else if (!endpoint.ParseFromIstream(&input)) {
        derror("File %s does not contain a valid endpoint",
               pathToEndpointProto.data());
        return nullptr;
    }

    return Builder()
            .withEndpoint(endpoint)
            .withInterceptors(std::move(factories))
            .build();
}

std::shared_ptr<EmulatorGrpcClient> sMe;
std::mutex sMeLock;

std::shared_ptr<EmulatorGrpcClient> EmulatorGrpcClient::me() {
    std::lock_guard<std::mutex> guard(sMeLock);
    if (!sMe) {
        dwarning(
                "Emulator client has not yet been configured.. Call configure "
                "me first!");
    }
    return sMe;
}

void EmulatorGrpcClient::configureMe(std::unique_ptr<EmulatorGrpcClient> me) {
    std::lock_guard<std::mutex> guard(sMeLock);
    sMe = std::move(me);
}

Builder& Builder::withInterceptor(ClientInterceptorFactoryInterface* factory) {
    mFactories.push_back(
            std::unique_ptr<ClientInterceptorFactoryInterface>(factory));
    return *this;
}

Builder& Builder::withInterceptors(InterceptorFactories factories) {
    mFactories = std::move(factories);
    return *this;
}

Builder& Builder::withDiscoveryFile(std::string discovery_file) {
    mDestination.Clear();
    android::base::IniFile iniFile(discovery_file);
    iniFile.read();
    if (!iniFile.hasKey("grpc.port")) {
        mStatus = absl::InvalidArgumentError("No grpc port defined in " +
                                             discovery_file);
        return *this;
    }

    if (iniFile.hasKey("grpc.server_cert")) {
        mStatus = absl::UnimplementedError("Tls enabled in " + discovery_file +
                                           " this is not supported");
        return *this;
    }
    if (iniFile.hasKey("grpc.token")) {
        auto token = iniFile.getString("grpc.token", "");
        auto basicTokenAuth = mDestination.add_required_headers();
        basicTokenAuth->set_key(BasicTokenAuth::DEFAULT_HEADER);
        basicTokenAuth->set_value("Bearer " + token);
    }
    mDestination.set_target("localhost:" +
                            iniFile.getString("grpc.port", "8554"));

    return *this;
}

Builder& Builder::withEndpoint(const Endpoint& destination) {
    mDestination.Clear();
    mDestination.CopyFrom(destination);
    return *this;
}

absl::StatusOr<std::unique_ptr<EmulatorGrpcClient>> Builder::build() {
    if (!mStatus.ok()) {
        return mStatus;
    }

    return std::unique_ptr<EmulatorGrpcClient>(
            new EmulatorGrpcClient(mDestination, std::move(mFactories)));
}
}  // namespace control
}  // namespace emulation
}  // namespace android
