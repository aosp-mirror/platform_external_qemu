// Copyright (C) 2023 The Android Open Source Project
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
#include "absl/status/status.h"

namespace android {
namespace emulation {
namespace control {

// A class that produces errors that are sensible to end users.
class AuthErrorFactory {
public:
    /**
     * Produces a UnauthenticatedError when there are no active keysets
     *
     * @param jwk_path The path where the public keys should be stored.
     * @return An absl::Status indicating failure of the operation.
     */
    static absl::Status authErrorNoKeySet(std::string_view jwk_path);


    /**
     * Produces a UnauthenticatedError when we presented a bad header
     *
     * @param header The header that the user presented.
     * @param expected What we expected to be there.
     * @return An absl::Status indicating failure of the operation.
     */
    static absl::Status authErrorInvalidHeader(std::string_view header, std::string_view expected);

    /**
     * Produces a UnauthenticatedError when we presented a bad token.
     *
     * @param token The token that the user presented.
     * @return An absl::Status indicating failure of the operation.
     */
    static absl::Status authErrorInvalidToken(std::string_view token);

    /**
     * Produces a UnauthenticatedError when we are unable to process the token
     *
     * @param path The path that the user tried to access.
     * @param token The token that the user presented.
     * @return An absl::Status indicating failure of the operation.
     */
    static absl::Status authErrorNoValidatorForToken(std::string_view path, std::string_view token);

    /**
     * Produces a UnauthenticatedError when the security header is not present
     *
     * @param header The header that is missing
     * @return An absl::Status indicating failure of the operation.
     */
    static absl::Status authErrorMissingHeader(std::string_view header);

    /**
     * Produces a UnauthenticatedError when an unidentified user
     * tries to access a path (iss field is missing)
     *
     * @param path The path that the user tried to access.
     * @return An absl::Status indicating failure of the operation.
     */
    static absl::Status authErrorMissingIss(std::string_view path);

    /**
     * Produces a PermissionDeniedError when a user tries to access a path
     * without having an "aud" entry
     *
     * @param iss The issuer. This is the user who will see the error
     * @param path The path that the user tried to access.
     * @return An absl::Status indicating failure of the operation.
     */
    static absl::Status authErrorMissingAud(std::string_view iss,
                                            std::string_view path);

    /**
     * Produces a PermissionDeniedError when a user tries to access a path
     * without having an "aud" claim to the path.
     *
     * * @param iss The issuer. This is the user who will see the error
     * @param path The path that the user tried to access.
     * @return An absl::Status indicating failure of the operation.
     */
    static absl::Status authErrorMissingClaim(std::string_view iss,
                                              std::string_view path);

    /**
     * Produces a PermissionDeniedError when a user tries to access a path
     * which is not on the allow list
     *
     * @param iss The issuer. This is the user who will see the error
     * @param path The path that the user tried to access.
     * @param allowListOrigin The location of the allowlist.
     * @return An absl::Status indicating failure of the operation.
     */
    static absl::Status authErrorNotOnAllowList(
            std::string_view iss,
            std::string_view path,
            std::string_view allowListOrigin);
};

}  // namespace control
}  // namespace emulation
}  // namespace android
