
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
#include "android/emulation/control/secure/AuthErrorFactory.h"
#include "absl/strings/str_format.h"

namespace android {
namespace emulation {
namespace control {

const std::string_view kGRADLE = "gradle-utp-emulator-control";
const std::string_view kSTUDIO = "android-studio";

absl::Status AuthErrorFactory::authErrorNoKeySet(std::string_view jwk_path) {
    return absl::UnauthenticatedError(absl::StrFormat(
            "No key set present. Make sure your public jwk is saved in %s",
            jwk_path));
}

absl::Status AuthErrorFactory::authErrorNoValidatorForToken(
        std::string_view path,
        std::string_view token) {
    return absl::UnauthenticatedError(
            absl::StrFormat("The token `%s` is invalid", token));
}

absl::Status AuthErrorFactory::authErrorInvalidToken(std::string_view token) {
    return absl::UnauthenticatedError(
            absl::StrFormat("The token `%s` is invalid", token));
}

absl::Status AuthErrorFactory::authErrorInvalidHeader(
        std::string_view header,
        std::string_view expected) {
    return absl::UnauthenticatedError(absl::StrFormat(
            "Invalid token header: `%s`, it does not start with: `%s`", header,
            expected));
}

absl::Status AuthErrorFactory::authErrorMissingHeader(std::string_view header) {
    return absl::UnauthenticatedError(absl::StrFormat(
            "Missing the '%s' header with security credentials.", header));
}

absl::Status AuthErrorFactory::authErrorMissingIss(std::string_view path) {
    return absl::UnauthenticatedError(
            "The JWT token is missing the iss claim.");
}

absl::Status AuthErrorFactory::authErrorMissingAud(std::string_view iss,
                                                   std::string_view path) {
    if (iss == kGRADLE) {
        // In the gradle case the user has not added the entry to the control
        // block
        return absl::PermissionDeniedError(
                absl::StrFormat("Make sure to add `allowedEndpoints.add(\"%s\")` "
                                "to the emulatorControl block in your gradle build file.",
                                path));
    }

    return absl::PermissionDeniedError(
            absl::StrFormat("The JWT does not have an aud claim. Make sure to "
                            "include: `\"aud\" : [\"%s\"]` in your JWT.",
                            path));
}

absl::Status AuthErrorFactory::authErrorMissingClaim(std::string_view iss,
                                                     std::string_view path) {
    if (iss == kGRADLE) {
        return absl::PermissionDeniedError(
                absl::StrFormat("Make sure to add `allowedEndpoints.add(\"%s\")` "
                                "to the emulatorControl block in your gradle build file.",
                                path));
    }

    return absl::PermissionDeniedError(
            absl::StrFormat("The JWT does not include %s in the aud claim. Make sure "
                            "to add it to the array.",
                            path));
}

absl::Status AuthErrorFactory::authErrorNotOnAllowList(
        std::string_view iss,
        std::string_view path,
        std::string_view allowListOrigin) {
    return absl::PermissionDeniedError(absl::StrFormat(
            "The endpoint: %s is not on the allowlist loaded from: "
            "`%s`. Add %s to the protected or allowed section of %s.",
            path, allowListOrigin, path, iss)

    );
}
}  // namespace control
}  // namespace emulation
}  // namespace android