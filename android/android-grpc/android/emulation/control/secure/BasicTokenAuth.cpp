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
#include "android/emulation/control/secure/BasicTokenAuth.h"

#include <map>      // for operator!=, __map_const_iterator
#include <string>   // for operator<
#include <utility>  // for make_pair, pair

namespace android {
namespace emulation {
namespace control {

BasicTokenAuth::BasicTokenAuth(std::string header, std::string prefix)
    : mHeader(header), mPrefix(prefix) {}

BasicTokenAuth::~BasicTokenAuth() = default;

grpc::Status BasicTokenAuth::Process(const InputMetadata& auth_metadata,
                                     grpc::AuthContext* context,
                                     OutputMetadata* consumed_auth_metadata,
                                     OutputMetadata* response_metadata) {
    auto header = auth_metadata.find(mHeader);
    if (header != auth_metadata.end() && header->second.starts_with(mPrefix)) {
        auto token = header->second.substr(mPrefix.size());
        if (isTokenValid(token)) {
            return grpc::Status::OK;
        }
    }

    return grpc::Status(grpc::StatusCode::UNAUTHENTICATED,
                        "You don't have permission to make this call.");
};

StaticTokenAuth::StaticTokenAuth(std::string token)
    : BasicTokenAuth(DEFAULT_HEADER, DEFAULT_BEARER), mStaticToken(token){};

bool StaticTokenAuth::isTokenValid(grpc::string_ref token) {
    return token == mStaticToken;
}
}  // namespace control
}  // namespace emulation
}  // namespace android
