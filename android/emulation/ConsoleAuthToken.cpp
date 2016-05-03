// Copyright 2016 The Android Open Source Project
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

#include "android/emulation/ConsoleAuthToken.h"

#include "android/base/EintrWrapper.h"
#include "android/base/files/PathUtils.h"
#include "android/base/files/ScopedStdioFile.h"
#include "android/base/files/ScopedFd.h"
#include "android/base/misc/Base64.h"
#include "android/base/misc/Random.h"
#include "android/base/StringFormat.h"
#include "android/base/StringView.h"
#include "android/base/system/System.h"
#include "android/emulation/ConfigDirs.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>

namespace android {

using android::base::System;
using android::base::StringView;
using android::base::StringFormat;

// TECHNICAL NOTE:
//
// Normally, all emulator-related configuration variables should be
// placed under $ANDROID_SDK_HOME/.android/ but the auth token file
// is an exception because it needs to be only readable for the current
// user, and in some configurations, $ANDROID_SDK_HOME/.android/ is
// actually shared between several ones/
//
// Due to this, the auth token file is always placed in the user's
// home directory instead.

ConsoleAuthToken::ConsoleAuthToken(OpenMode openMode) {
    System* system = System::get();

    std::string userDir = system->getHomeDirectory();
    if (userDir.empty()) {
        mStatus = "Cannot determine user home directory!";
        return;
    }
    if (!system->pathExists(userDir)) {
        mStatus = "User home directory does not exist";
        return;
    }

    std::string authFilePath =
            android::base::PathUtils::join(userDir, kFileName);

    if (openMode == OpenMode::CreateOnDemand) {

        // It's important that the auth file is only visible to the user.
        mode_t kAuthFileMode = 0600;

        // First try to create + open the file, this operation will fail
        // with EEXIST if the file already exists.
        const int kAuthFileFlags = O_CREAT | O_TRUNC | O_EXCL | O_WRONLY;
        android::base::ScopedFd fd(system->pathFileOpen(
                authFilePath, kAuthFileFlags, kAuthFileMode));
        if (fd.get() < 0) {
            if (errno != EEXIST) {
                mStatus = StringFormat(
                        "Could not create console auth token file: %s",
                        strerror(errno));
                return;
            }
#ifndef _WIN32
            // The file exists, ensure it can only be read by the current user.
            // Windows doesn't support 'group' and 'other' permissions so this
            // check is not necessary.
            System::PathStat st = {};
            if (system->pathFileStat(authFilePath, &st) < 0) {
                mStatus = StringFormat(
                        "Could not access console auth token file: %s",
                        strerror(errno));
                return;
            }
            const mode_t kExpectedFileMode = kAuthFileMode | S_IFREG;
            if (st.st_mode != kExpectedFileMode) {
                mStatus = StringFormat(
                        "Invalid console auth token file mode: %0o expected "
                        "%0o",
                        st.st_mode, kExpectedFileMode);
                return;
            }
#endif  // !_WIN32
        } else {
            // The file was just created. Generate new value and write it into
            // it.
            uint8_t tokenBytes[kSize] = {};
            android::genRandomBytes(tokenBytes, sizeof(tokenBytes));
            std::string token64 = android::base64Encode(tokenBytes, kSize);

            ssize_t ret = HANDLE_EINTR(
                    ::write(fd.get(), token64.c_str(), token64.size()));
            if (ret < 0) {
                mStatus = StringFormat(
                        "Could not write console auth token file: %s",
                        strerror(errno));
                return;
            }
            fd.close();
        }
    }

    android::base::ScopedStdioFile file(
            system->pathFileOpenStdio(authFilePath, "rb"));
    if (!file.get()) {
        mStatus = StringFormat("Could not open console auth token: %s",
                               strerror(errno));
        return;
    }

    // Expected format: base64-encoded stream of kSize bytes.
    const size_t bufferLen = android::base64EncodedSizeForInputSize(kSize);
    uint8_t buffer[bufferLen + 1] = {};

    size_t ret = ::fread(buffer, 1, bufferLen, file.get());
    if (ret == 0 && ::feof(file.get())) {
        // An empty file means all tokens are accepted by default.
        mPassThrough = true;
        mValid = true;
        return;
    }
    if (ret != bufferLen) {
        mStatus = "Console authentication file is too short to be valid";
        return;
    }

    android::base::Optional<std::string> data =
            android::base64Decode(buffer, bufferLen);
    if (!data) {
        mStatus = "Console auth token content is not valid base64!";
        return;
    }
    if (data->size() != kSize) {
        mStatus = StringFormat(
                "Console auth token has not valid length (%d expected %d)",
                (int)data->size(), (int)kSize);
        return;
    }
    ::memcpy(mBytes, data->c_str(), kSize);

    // Read the file now.
    mValid = true;
}

std::string ConsoleAuthToken::toString() const {
    if (!mValid) {
        return std::string("<invalid-auth-token>");
    }
    if (mPassThrough) {
        return std::string();
    }
    return android::base64Encode(
            StringView(reinterpret_cast<const char*>(mBytes), kSize));
}

bool ConsoleAuthToken::checkAgainst(android::base::StringView str) {
    if (!mValid) {
        return false;
    }
    if (mPassThrough) {
        return true;
    }
    android::base::Optional<std::string> data = android::base64Decode(str);
    if (!data) {
        // Invalid base64 string.
        return false;
    }
    if (data->size() != kSize || ::memcmp(mBytes, data->c_str(), kSize)) {
        return false;
    }
    return true;
}

// static
const StringView ConsoleAuthToken::kFileName = ".emulator_console_auth_token";

}  // namespace android
