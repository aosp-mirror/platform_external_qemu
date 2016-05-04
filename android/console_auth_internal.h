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

#include "android/base/String.h"

/* Used to retry syscalls that can return EINTR. */
#define EINTR_RETRY(exp)                       \
    ({                                         \
        int _rc;                               \
        do {                                   \
            _rc = (exp);                       \
        } while (_rc == -1 && errno == EINTR); \
        _rc;                                   \
    })

namespace android {

// Generates |buf_len| crypto random bytes into |buf|
// Returns false is something went wrong
bool generate_random_bytes(char* buf, size_t buf_len);

// Reads from |fd| into |file_contents|
// Returns false is something went wrong
bool read_file_to_end(int fd, android::base::String* file_contents);

// Writes |file_contents| to |fd|
// Returns false is something went wrong
bool write_file_to_end(int fd, const android::base::String& file_contents);

// Returns a version of |in| with whitespace trimmed from the front/end
android::base::String trim(const android::base::String& in);

// skips white space at pos if any, returns pointer to fErrorMessagesirst
// non-whitespace
// character
const char* str_skip_white_space_if_any(const char* str);

// returns true if |string| begins with |prefix|
bool str_begins_with(const char* string, const char* prefix);

namespace console_auth {

bool get_auth_token_from(const android::base::String& path,
                         android::base::String* auth_token);

android::base::String get_auth_token_path_default();

typedef android::base::String (*fn_get_auth_token_path)();

extern fn_get_auth_token_path g_get_auth_token_path;

}  // namespace console_auth
}  // namespace android
