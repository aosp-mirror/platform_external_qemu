// Copyright (C) 2022 The Android Open Source Project
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
#include "android/emulation/control/secure/JwtTokenAuth.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <utility>

#include "absl/strings/string_view.h"
#include "aemu/base/files/PathUtils.h"
#include "aemu/base/misc/StringUtils.h"
#include "android/emulation/control/secure/AuthErrorFactory.h"
#include "android/utils/debug.h"
#include "tink/config/tink_config.h"
#include "tink/jwt/jwk_set_converter.h"
#include "tink/jwt/jwt_public_key_verify.h"
#include "tink/jwt/jwt_signature_config.h"
#include "tink/jwt/jwt_validator.h"
#include "tink/util/status.h"
#include "tink/util/statusor.h"

// #define DEBUG 1

#if DEBUG >= 1
#define DD(fmt, ...) \
    printf("JwtTokenAuth: %s:%d| " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#else
#define DD(...) (void)0
#endif

namespace android {
namespace emulation {
namespace control {
using android::base::PathUtils;

namespace tink = crypto::tink;

JwtTokenAuth::JwtTokenAuth(Path jwksPath, Path jwksLoadedPath, AllowList* list)
    : BasicTokenAuth(DEFAULT_HEADER, list), mJwksLoadedPath(jwksLoadedPath) {
    mTinkInitialized = tink::TinkConfig::Register();
    if (mTinkInitialized.ok()) {
        mTinkInitialized = tink::JwtSignatureRegister();
    }

    if (!mJwksLoadedPath.empty()) {
        dinfo("The active JSON Web Key Sets can be found here: %s",
              mJwksLoadedPath.c_str());
    }

    if (!mTinkInitialized.ok()) {
        dfatal("Unable to initalize tink library. %s",
               mTinkInitialized.ToString().c_str());
    }
    mDirectoryObserver = std::make_unique<JwkDirectoryObserver>(
            jwksPath,
            [&](auto handle) { updateKeysetHandle(std::move(handle)); },
            [&](auto fname) {
                DD("Check %s  != %s", fname.c_str(), mJwksLoadedPath.c_str());
                return android::base::EndsWith(fname, kJwkExt) &&
                       fname != mJwksLoadedPath;
            });
}

void JwtTokenAuth::updateKeysetHandle(
        std::unique_ptr<crypto::tink::KeysetHandle> incomingHandle) {
    const std::lock_guard<std::mutex> lock(mKeyhandleAccess);
    if (mTinkInitialized.ok())
        mActiveKeyset = std::move(incomingHandle);

    // Next we serialize the Jwkset to file.
    if (mTinkInitialized.ok() && !mJwksLoadedPath.empty()) {
        DD("Updating active set %p", mActiveKeyset.get());
        std::string jsonSnippet = R"({"keys": []})";
        if (mActiveKeyset != nullptr) {
            auto jwkSet = tink::JwkSetFromPublicKeysetHandle(*mActiveKeyset);
            if (!jwkSet.ok()) {
                DD("Cannot serialize jwk set %s",
                   jwkSet.status().message().data());
            } else {
                jsonSnippet = jwkSet.value();
            }
        }

        std::ofstream out(
                PathUtils::asUnicodePath(mJwksLoadedPath.data()).c_str(),
                std::ios::trunc);
        out << jsonSnippet;
        out.close();
        DD("Updated %s with latest loaded keys to: %s", mJwksLoadedPath.c_str(),
           jsonSnippet.c_str());
    }
}

bool JwtTokenAuth::canHandleToken(std::string_view token) {
    constexpr auto offset = DEFAULT_BEARER.size();
    if (token.size() <= offset) {
        return false;
    }

    // See https://datatracker.ietf.org/doc/html/rfc7519#section-3
    // Base64url(JOSE Header) + '.' + Base64url(claims) + '.' +
    // Base64url(signature)
    return std::count(token.begin(), token.end(), '.') == 2;
}

absl::Status JwtTokenAuth::isTokenValid(std::string_view path,
                                        std::string_view token) {
    if (!mTinkInitialized.ok()) {
        return mTinkInitialized;
    }

    constexpr auto offset = DEFAULT_BEARER.size();
    if (token.size() <= offset) {
        return AuthErrorFactory::authErrorInvalidHeader(token, DEFAULT_BEARER);
    }

    auto actual_token =
            absl::string_view(token.data() + offset, token.size() - offset);

    const std::lock_guard<std::mutex> lock(mKeyhandleAccess);
    if (!mActiveKeyset) {
        return AuthErrorFactory::authErrorNoKeySet(mDirectoryObserver->observes());
    }
    auto verify =
            mActiveKeyset->template GetPrimitive<tink::JwtPublicKeyVerify>();
    if (!verify.ok()) {
        DD("Token error: %s", verify.status().error_message().c_str());
        return verify.status();
    }

    DD("Make sure there is a proper signed jwt token.");
    auto validator = tink::JwtValidatorBuilder()
                             .ExpectIssuedInThePast()
                             .IgnoreIssuer()
                             .IgnoreAudiences()
                             .Build();

    if (!validator.ok()) {
        DD("Token validator error: %s",
           validator.status().error_message().c_str());
        return validator.status();
    }

    auto verified_jwt = (*verify)->VerifyAndDecode(actual_token, *validator);

    // Bad token, this means we have no way of doing anything useful.
    if (!verified_jwt.ok()) {
        DD("Bad token!: %s", verified_jwt.status().error_message().c_str());
        return verified_jwt.status();
    }

    auto iss = verified_jwt->GetIssuer();
    if (!iss.ok()) {
        return AuthErrorFactory::authErrorMissingIss(path);
    }

    // Green list, no need to validate aud..
    if (verified_jwt.ok() && allowList()->isAllowed(*iss, path)) {
        return absl::OkStatus();
    }

    // On the block list, go away!
    if (allowList()->isRed(*iss, path)) {
        return AuthErrorFactory::authErrorNotOnAllowList(
                *iss, path, allowList()->getSource());
    }

    auto audiences = verified_jwt->GetAudiences();
    if (!audiences.ok()) {
        DD("Token validator error: %s",
           audiences.status().error_message().c_str());
        return AuthErrorFactory::authErrorMissingAud(*iss, path);
    }

    for (const auto& aud : audiences.value()) {
        if (aud == path) {
            return absl::OkStatus();
        }
    }

    return AuthErrorFactory::authErrorMissingClaim(*iss, path);
}

}  // namespace control
}  // namespace emulation
}  // namespace android
