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

#pragma once

#include "android/base/StringView.h"

#include <string>

#include <stddef.h>
#include <stdint.h>

namespace android {

// A convenience class used to read the content of the Android console
// emulator authentication token. Usage is the following:
//
// 1) Create new instance, this will read the filesystem for the token's value.
//
// 2) Call isValid() to check that the file existed and contained appropriate
//    values (see note on format below).
//
// 3) Call checkAgainst() to check a user-provided string against the token.
//
class ConsoleAuthToken {
public:
    // Size of the auth token in bytes.
    static constexpr size_t kSize = 16;

    // Open mode for the constructor.
    // CreateOnDemand is the default and means to create the file if it
    // doesn't already exist.
    // FailIfMissing means that construction should result in an invalid
    // token object if if could not be found on the file system.
    enum class OpenMode {
        CreateOnDemand,
        FailIfMissing,
    };

    // Default constructor will try to read the content of the auth token
    // from the file system.
    explicit ConsoleAuthToken(OpenMode openMode = OpenMode::CreateOnDemand);

    ~ConsoleAuthToken() = default;

    // Returns true if the token could be read and is well-formed. Returns
    // false in case of error, call getError() to know why.
    bool isValid() const { return mValid; }

    // Returns a string describing the token's status, i.e. If isValid() returns
    // false, return a string describing the error.
    std::string getStatus() const { return mStatus; }

    // Convert to hexchar string.
    std::string toString() const;

    // Check a user-provided string |str| against the token. This returns true
    // if this matches the hex-char encoded token bytes, or if the token value
    // is the special 'passthrough' value that accepts anything (and disables
    // security).
    bool checkAgainst(android::base::StringView str);

private:
    uint8_t mBytes[kSize] = {};
    std::string mStatus;
    bool mValid = false;
    bool mPassThrough = false;
};

}  // namespace android
