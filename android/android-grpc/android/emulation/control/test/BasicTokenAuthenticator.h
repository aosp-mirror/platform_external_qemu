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
#include <grpcpp/grpcpp.h>  // for Status

#include <string>  // for string

#include "android/emulation/control/secure/BasicTokenAuth.h"

namespace android {
namespace emulation {
namespace control {

using ::android::emulation::control::BasicTokenAuth;

// A credentials plugin that injects the given token when making a gRPC call.
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
        printf("Getting metadata\n");
        metadata->insert(
                std::make_pair(BasicTokenAuth::DEFAULT_HEADER, mToken));
        return grpc::Status::OK;
    }

private:
    grpc::string mToken;
};
}  // namespace control
}  // namespace emulation
}  // namespace android
