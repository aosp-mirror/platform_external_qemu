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
#pragma once
#include <grpcpp/grpcpp.h>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>

#include "absl/status/status.h"
#include "android/emulation/control/secure/AllowList.h"
#include "android/emulation/control/secure/BasicTokenAuth.h"
#include "android/emulation/control/secure/JwkDirectoryObserver.h"
#include "tink/keyset_handle.h"

namespace android {
namespace emulation {
namespace control {

using Path = std::string;

// A class that validates that the header:
//
// authorization: Bearer <token>
//
// is present and contains a valid JWT token.
//
// JWT tokens will be validated on the basis of a set of JWK files that can
// be found in the jkwsPath.
//
// Tokens must contain the following:
//
// An 'iss' field, that must be on the allow list.
// An 'aud' field matching the 'path' of the method invoked if 
// the 'path' is not on the green list.
// The 'exp' field is not yet expired.
class JwtTokenAuth : public BasicTokenAuth {
public:
    JwtTokenAuth(Path jwksPath,
                 Path jwksLoadedPath,
                 AllowList* list);
    ~JwtTokenAuth() = default;
    absl::Status isTokenValid(grpc::string_ref path,
                              grpc::string_ref token) override;

private:
    void updateKeysetHandle(
            std::unique_ptr<crypto::tink::KeysetHandle> incomingHandle);

    static constexpr const std::string_view DEFAULT_BEARER{"Bearer "};
    static constexpr const std::string_view kJwkExt{".jwk"};
    Path mJwksLoadedPath;
    std::mutex mKeyhandleAccess;
    absl::Status mTinkInitialized;
    std::unique_ptr<crypto::tink::KeysetHandle> mActiveKeyset;
    std::unique_ptr<JwkDirectoryObserver> mDirectoryObserver;
};
}  // namespace control
}  // namespace emulation
}  // namespace android
