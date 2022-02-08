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

#include <map>                        // for operator==, __map_const_iterator
#include <string>                     // for string
#include <utility>                    // for move, pair

#include "absl/strings/str_format.h"  // for StrFormat
#include "android/utils/debug.h"

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

    auto path = auth_metadata.find(PATH);
    if (path == auth_metadata.end()) {
        return grpc::Status(grpc::StatusCode::INTERNAL,
                            "The metadata does not contain a path!");
    }

    auto header = auth_metadata.find(mHeader);
    if (header == auth_metadata.end()) {
        return grpc::Status(
                grpc::StatusCode::UNAUTHENTICATED,
                absl::StrFormat("No '%s' header present.", mHeader));
    }

    if (!header->second.starts_with(mPrefix)) {
        return grpc::Status(
                grpc::StatusCode::UNAUTHENTICATED,
                absl::StrFormat("Header does not start with '%s'.", mPrefix));
    }

    auto token = header->second.substr(mPrefix.size());
    auto auth = isTokenValid(path->second, token);
    return auth.ok() ? grpc::Status::OK
                     : grpc::Status(grpc::StatusCode::UNAUTHENTICATED,
                                    (std::string)auth.message());
};

StaticTokenAuth::StaticTokenAuth(std::string token)
    : BasicTokenAuth(DEFAULT_HEADER, DEFAULT_BEARER), mStaticToken(token){
        dwarning("*** Basic token auth will be deprecated soon, please migrate to using -grpc-use-jwt ***");
    };

absl::Status StaticTokenAuth::isTokenValid(grpc::string_ref url,
                                           grpc::string_ref token) {
    return token == mStaticToken
                   ? absl::OkStatus()
                   : absl::PermissionDeniedError("Incorrect token");
}
AnyTokenAuth::AnyTokenAuth(
        std::vector<std::unique_ptr<BasicTokenAuth>> validators)
    : BasicTokenAuth(DEFAULT_HEADER), mValidators(std::move(validators)) {}

absl::Status AnyTokenAuth::isTokenValid(grpc::string_ref path,
                                        grpc::string_ref token) {
    for (const auto& validator : mValidators) {
        if (validator->isTokenValid(path, token).ok()) {
            return absl::OkStatus();
        }
    }
    return absl::PermissionDeniedError("Token rejected by all validators.");
}

}  // namespace control
}  // namespace emulation
}  // namespace android
