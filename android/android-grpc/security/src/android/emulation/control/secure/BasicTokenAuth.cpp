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

#include <map>      // for operator==, __map_const_iterator
#include <string>   // for string
#include <utility>  // for move, pair

#include "absl/strings/str_format.h"  // for StrFormat
#include "android/utils/debug.h"

namespace android {
namespace emulation {
namespace control {

DisableAccess BasicTokenAuth::noAccess;

BasicTokenAuth::BasicTokenAuth(std::string header, AllowList* list)
    : mHeader(header), mAllowList(list) {
    }

BasicTokenAuth::~BasicTokenAuth() = default;

grpc::Status BasicTokenAuth::Process(const InputMetadata& auth_metadata,
                                     grpc::AuthContext* context,
                                     OutputMetadata* consumed_auth_metadata,
                                     OutputMetadata* response_metadata) {
    auto path = auth_metadata.find(PATH);
    if (path == auth_metadata.end()) {
        return grpc::Status(grpc::StatusCode::INTERNAL,
                            "The metadata does not contain a path");
    }

    auto header = auth_metadata.find(mHeader);
    if (header == auth_metadata.end()) {
        return grpc::Status(
                grpc::StatusCode::UNAUTHENTICATED,
                absl::StrFormat("No '%s' header present.", mHeader));
    }

    auto auth = isTokenValid(path->second, header->second);
    return auth.ok() ? grpc::Status::OK
                     : grpc::Status(grpc::StatusCode::UNAUTHENTICATED,
                                    (std::string)auth.message());
};

StaticTokenAuth::StaticTokenAuth(std::string token,
                                 std::string iss,
                                 AllowList* list)
    : BasicTokenAuth(DEFAULT_HEADER, list),
      mStaticToken(DEFAULT_BEARER + token),
      mIssuer(iss) {
    dwarning(
            "*** Basic token auth will be deprecated soon, and access points will be limited. ***");
};

absl::Status StaticTokenAuth::isTokenValid(grpc::string_ref url,
                                           grpc::string_ref token) {
    std::string_view path(url.data(), url.length());

    if (!allowList()->requiresAuthentication(path)) {
        return absl::OkStatus();
    }


    auto status = token == mStaticToken
                          ? absl::OkStatus()
                          : absl::PermissionDeniedError("Incorrect token");
    if (status.ok() && allowList()->isRed(mIssuer, path)) {
        return absl::PermissionDeniedError("Access is blocked by access list");
    }
    if (!status.ok()) {
        dwarning("%s != %s", mStaticToken.c_str(), token.data());
    }

    return status;
}
AnyTokenAuth::AnyTokenAuth(
        std::vector<std::unique_ptr<BasicTokenAuth>> validators,
        AllowList* allowlist)
    : BasicTokenAuth(DEFAULT_HEADER, allowlist), mValidators(std::move(validators)) {
}

absl::Status AnyTokenAuth::isTokenValid(grpc::string_ref url,
                                        grpc::string_ref token) {

    std::string_view path(url.data(), url.length());
    if (mValidators.empty() || !allowList()->requiresAuthentication(path)) {
        return absl::OkStatus();
    }

    std::string error_message = "Validation failed: [";
    for (const auto& validator : mValidators) {
        auto status = validator->isTokenValid(url, token);
        if (status.ok()) {
            return absl::OkStatus();
        }
        error_message += "\"" + std::string(status.message()) + "\", ";
    }
    error_message += "]";
    return absl::PermissionDeniedError(error_message);
}

}  // namespace control
}  // namespace emulation
}  // namespace android
