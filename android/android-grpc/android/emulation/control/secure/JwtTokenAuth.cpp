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

#include <functional>  // for __base
#include <utility>     // for move

#include "absl/strings/string_view.h"        // for string_view
#include "android/utils/debug.h"             // for derror
#include "tink/config/tink_config.h"         // for TinkConfig
#include "tink/jwt/jwt_public_key_verify.h"  // for JwtPublicKeyVerify
#include "tink/jwt/jwt_signature_config.h"   // for JwtSignatureRegister
#include "tink/jwt/jwt_validator.h"          // for JwtValidatorBuilder
#include "tink/util/status.h"                // for Status
#include "tink/util/statusor.h"              // for StatusOr

#define DEBUG 0

#if DEBUG >= 1
#define DD(fmt, ...)                                                     \
    printf("JwkDirectoryObserver: %s:%d| " fmt "\n", __func__, __LINE__, \
           ##__VA_ARGS__)
#else
#define DD(...) (void)0
#endif

namespace android {
namespace emulation {
namespace control {

namespace tink = crypto::tink;

JwtTokenAuth::JwtTokenAuth(Path jwksPath)
    : BasicTokenAuth(DEFAULT_HEADER, DEFAULT_BEARER),
      mDirectoryObserver(jwksPath, [&](auto handle) {
          updateKeysetHandle(std::move(handle));
      }) {
    mTinkInitialized = tink::TinkConfig::Register();
    if (mTinkInitialized.ok()) {
        mTinkInitialized = tink::JwtSignatureRegister();
    }
}

void JwtTokenAuth::updateKeysetHandle(
        std::unique_ptr<crypto::tink::KeysetHandle> incomingHandle) {
    const std::lock_guard<std::mutex> lock(mKeyhandleAccess);
    if (mTinkInitialized.ok())
        mActiveKeyset = std::move(incomingHandle);
}

absl::Status JwtTokenAuth::isTokenValid(grpc::string_ref path, grpc::string_ref token) {
    if (!mTinkInitialized.ok()) {
        return mTinkInitialized;
    }

    const std::lock_guard<std::mutex> lock(mKeyhandleAccess);
    if (!mActiveKeyset) {
        return absl::NotFoundError("No key handles, rejecting all tokens.");
    }
    auto verify =
            mActiveKeyset->template GetPrimitive<tink::JwtPublicKeyVerify>();
    if (!verify.ok()) {
        return verify.status();
    }

    absl::string_view method(path.data(), path.size());
    DD("Validating aud: %s", method.data());
    auto validator =
            tink::JwtValidatorBuilder()
                    .ExpectAudience(method)
                    .ExpectIssuedInThePast()
                    .IgnoreIssuer()
                    .Build();
    if (!validator.ok()) {
        return validator.status();
    }

    auto verified_jwt = (*verify)->VerifyAndDecode(
            absl::string_view(token.data(), token.size()), *validator);
    return verified_jwt.status();
}

}  // namespace control
}  // namespace emulation
}  // namespace android
