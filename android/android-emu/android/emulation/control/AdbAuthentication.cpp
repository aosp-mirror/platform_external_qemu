// Copyright (C) 2019 The Android Open Source Project
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
#include "android/emulation/control/AdbAuthentication.h"

#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <string>
#include <vector>
#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"

/* set to 1 for very verbose debugging */
#define DEBUG 0

#if DEBUG >= 1
#define DD(fmt, ...) \
    printf("AdbAuth: %s:%d| " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#else
#define DD(...) (void)0
#endif

// Adb authentication
#define TOKEN_SIZE 20

namespace android {
namespace emulation {
static bool read_key(const char* file, std::vector<RSA*>& keys) {
    FILE* fp = fopen(file, "r");
    if (!fp) {
        DD("Failed to open '%s': %s", file, strerror(errno));
        return false;
    }

    RSA* key_rsa = RSA_new();

    if (!PEM_read_RSAPrivateKey(fp, &key_rsa, NULL, NULL)) {
        DD("Failed to read key");
        fclose(fp);
        RSA_free(key_rsa);
        return false;
    }

    keys.push_back(key_rsa);
    fclose(fp);
    return true;
}

static bool sign_token(RSA* key_rsa,
                       const char* token,
                       int token_size,
                       char* sig,
                       int& len) {
    if (token_size != TOKEN_SIZE) {
        DD("Unexpected token size %d\n", token_size);
    }

    if (!RSA_sign(NID_sha1, (const unsigned char*)token, (size_t)token_size,
                  (unsigned char*)sig, (unsigned int*)&len, key_rsa)) {
        return false;
    }

    DD("successfully signed with siglen %d\n", (int)len);
    return true;
}

bool sign_auth_token(const char* token,
                     int token_size,
                     char* sig,
                     int& siglen) {
    // read key
    using android::base::pj;
    std::vector<RSA*> keys;
    std::string key_path = pj(android::base::System::get()->getHomeDirectory(),
                              ".android", "adbkey");
    if (!read_key(key_path.c_str(), keys)) {
        // TODO: test the windows code path
        std::string key_path =
                pj(android::base::System::get()->getAppDataDirectory(),
                   ".android", "adbkey");
        if (!read_key(key_path.c_str(), keys)) {
            return false;
        }
    }
    return sign_token(keys[0], token, token_size, sig, siglen);
}

}  // namespace emulation
}  // namespace android
