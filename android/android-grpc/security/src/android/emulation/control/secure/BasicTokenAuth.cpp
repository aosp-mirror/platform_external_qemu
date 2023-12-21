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
#include <grpc++/grpc++.h>
#include <algorithm>
#include <map>
#include <numeric>
#include <string>
#include <utility>

#include "absl/strings/str_format.h"
#include "android/emulation/control/secure/AuthErrorFactory.h"
#include "android/utils/debug.h"

namespace android {
namespace emulation {
namespace control {

DisableAccess BasicTokenAuth::noAccess;
const std::string_view kSTUDIO = "android-studio";

/**
 * Convert a Abseil status object to an gRPC status object.
 * (Note: gRPC will eventually migrate to abseil..)
 *
 * @param absl_status The Abseil status object to convert.
 * @return The equivalent Abseil status object.
 */
static grpc::Status ConvertAbseilStatusToGrpcStatus(
        const absl::Status& absl_status) {
    return grpc::Status(static_cast<grpc::StatusCode>(absl_status.code()),
                        std::string(absl_status.message()));
}

BasicTokenAuth::BasicTokenAuth(std::string header, AllowList* list)
    : mHeader(header), mAllowList(list) {}

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

    std::string_view uri(path->second.data(), path->second.length());

    // First lets see if we even need to validate this uri.
    if (!allowList()->requiresAuthentication(uri)) {
        return grpc::Status::OK;
    }

    auto header = auth_metadata.find(mHeader);
    if (header == auth_metadata.end()) {
        return ConvertAbseilStatusToGrpcStatus(
                AuthErrorFactory::authErrorMissingHeader(mHeader));
    }

    std::string_view token(header->second.data(), header->second.length());
    if (!canHandleToken(token)) {
        return ConvertAbseilStatusToGrpcStatus(
                AuthErrorFactory::authErrorNoValidatorForToken(uri, token));
    }

    auto auth = isTokenValid(uri, token);
    return ConvertAbseilStatusToGrpcStatus(auth);
};

StaticTokenAuth::StaticTokenAuth(std::string token,
                                 std::string iss,
                                 AllowList* list)
    : BasicTokenAuth(DEFAULT_HEADER, list),
      mStaticToken(DEFAULT_BEARER + token),
      mIssuer(iss) {
    dwarning("*** Basic token auth should only be used by android-studio ***");
};

bool StaticTokenAuth::canHandleToken(std::string_view token) {
    return token == mStaticToken;
}

absl::Status StaticTokenAuth::isTokenValid(std::string_view path,
                                           std::string_view token) {
    if (!canHandleToken(token)) {
        // This can be very verbose..
        VERBOSE_PRINT(grpc, "%s != %s", mStaticToken, token);
        return AuthErrorFactory::authErrorInvalidToken(token);
    }

    if (allowList()->isRed(mIssuer, path)) {
        return AuthErrorFactory::authErrorNotOnAllowList(
                kSTUDIO, path, allowList()->getSource());
    }

    return absl::OkStatus();
}

AnyTokenAuth::AnyTokenAuth(
        std::vector<std::unique_ptr<BasicTokenAuth>> validators,
        AllowList* allowlist)
    : BasicTokenAuth(DEFAULT_HEADER, allowlist),
      mUniqueValidators(std::move(validators)) {
    for (const auto& validator : mUniqueValidators) {
        mValidators.push_back(validator.get());
    }
}

AnyTokenAuth::AnyTokenAuth(std::vector<BasicTokenAuth*> validators,
                           AllowList* allowlist)
    : BasicTokenAuth(DEFAULT_HEADER, allowlist),
      mValidators(std::move(validators)) {}

bool AnyTokenAuth::canHandleToken(std::string_view token) {
    if (mValidators.empty()) {
        return true;
    }

    for (const auto& validator : mValidators) {
        if (validator->canHandleToken(token)) {
            return true;
        }
    }

    return false;
}

absl::Status AnyTokenAuth::isTokenValid(std::string_view path,
                                        std::string_view token) {
    if (mValidators.empty()) {
        return absl::OkStatus();
    }

    // This should only be called when at least one validator is willing to
    // process this token.
    for (const auto& validator : mValidators) {
        if (validator->canHandleToken(token)) {
            return validator->isTokenValid(path, token);
        }
    }

    // Oh oh! How did we end up here??
    std::string validators = std::accumulate(
            mValidators.begin(), mValidators.end(), std::string(),
            [](auto str, const auto& validator) {
                return std::move(str) + ", " + validator->name();
            });

    auto fatalError = absl::StrFormat(
            "FATAL: No validator that can handle token. This "
            "should not happen, please file a bug including "
            "this information: Validation failure `validators: "
            "%s`, `path: %s`, `token: %s`",
            validators, path, token);

    derror("Internal error! %s", fatalError);

    // Should not happen, at least one validator should have been able
    // to handle the token.
    return absl::InternalError(fatalError);
}

}  // namespace control
}  // namespace emulation
}  // namespace android
