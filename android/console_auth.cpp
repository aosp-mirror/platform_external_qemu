// Copyright (C) 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/console_auth.h"
#include "android/console_auth_internal.h"

#include "android/base/system/System.h"
#include "android/base/files/ScopedFd.h"
#include "android/base/files/ScopedStdioFile.h"
extern "C" {
#include "android/proxy/proxy_int.h"
}
#include "android/utils/debug.h"
#include "android/utils/path.h"
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#ifdef _WIN32
#include <wincrypt.h>
#endif

#define E(...) derror(__VA_ARGS__)

using android::base::String;
using android::base::System;
using android::base::ScopedFd;
using android::base::ScopedStdioFile;

namespace android {

//==============================================================================
// General utility functions
//==============================================================================

bool generate_random_bytes(char* buf, size_t buf_len) {
#ifdef _WIN32
    HCRYPTPROV hCryptProv;
    DWORD dwFlags = CRYPT_VERIFYCONTEXT | CRYPT_SILENT;
    if (!CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, dwFlags)) {
        if (GetLastError() == NTE_BAD_KEYSET) {
            if (!CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL,
                                     dwFlags | CRYPT_NEWKEYSET)) {
                return false;
            }
        }
    }

    BOOL ok = CryptGenRandom(hCryptProv, buf_len, (BYTE*)buf);

    CryptReleaseContext(hCryptProv, 0);

    if (!ok) {
        E("Can't read from CryptGenRandom");
        return false;
    }
    return true;
#else
    ScopedStdioFile fp(fopen("/dev/urandom", "rb"));
    if (fp.get() == nullptr) {
        E("Can't open /dev/urandom");
        return false;
    }
    size_t err = fread(buf, 1, buf_len, fp.get());
    if (err != buf_len) {
        E("Can't read from /dev/urandom");
        return false;
    }
    return true;
#endif
}

bool read_file_to_end(int fd, String* file_contents) {
    off_t size = lseek(fd, 0, SEEK_END);
    if (size == (off_t)-1) {
        return false;
    }
    off_t err = lseek(fd, 0, SEEK_SET);
    if (err == (off_t)-1) {
        return false;
    }

    std::vector<char> buf(size);
    int result = EINTR_RETRY(read(fd, &buf[0], size));
    if (result != size) {
        return false;
    }
    file_contents->assign(&buf[0], result);
    return true;
}

bool write_file_to_end(int fd, const String& file_contents) {
    ssize_t result =
            EINTR_RETRY(write(fd, file_contents.c_str(), file_contents.size()));
    if (result != (ssize_t)file_contents.size()) {
        return false;
    }
    return true;
}

String trim(const String& in) {
    size_t start = 0;
    while (start < in.size() && isspace(in[start])) {
        start++;
    }

    size_t end = in.size();
    while (end > start && isspace(in[end - 1])) {
        end--;
    }
    return String(in.c_str() + start, end - start);
}

const char* str_skip_white_space_if_any(const char* pos) {
    while (isspace(*pos)) {
        pos++;
    }
    return pos;
}

bool str_begins_with(const char* string, const char* prefix) {
    return strncmp(prefix, string, strlen(prefix)) == 0;
}

//==============================================================================
// console_auth functions
//==============================================================================

namespace console_auth {

// creates a string suitable for use as an authentication token
// (text that is hard to guess)
bool generate_auth_token(String* auth_token) {
    char buf[16];  // 128 bit
    if (!android::generate_random_bytes(buf, sizeof(buf))) {
        return false;
    }

    const size_t BASE64_LEN = 4 * ((sizeof(buf) + 2) / 3);
    char base64[BASE64_LEN];
    int err = proxy_base64_encode(buf, sizeof(buf), base64, BASE64_LEN);
    assert(err == BASE64_LEN);
    if (err < 0) {
        E("proxy_base64_encode failed.  (What?)");
        return false;
    }

    auth_token->assign(base64, BASE64_LEN);
    return true;
}

// reads auth token from the specified path
// if the file does not exist, creates a file at the specified path
// and writes an auth token to it
//
// |path| is a separate parameter to make it unit testable
bool get_auth_token_from(const String& path, String* auth_token) {
    // it is critical that this file only be readable by the current user
    // to prevent other users on the system from reading it and gaining access
    // to the emulator console
    const mode_t user_read_only = 0600;

    // this open will fail if the file already exists
    ScopedFd fd(EINTR_RETRY(open(path.c_str(),
                                 O_CREAT | O_EXCL | O_WRONLY | O_TRUNC | O_BINARY,
                                 user_read_only)));
    if (fd.get() < 0) {
        ScopedFd read(EINTR_RETRY(open(path.c_str(), O_RDONLY | O_BINARY)));
        // file already exists, read it
        String file_contents;

        if (!android::read_file_to_end(read.get(), &file_contents)) {
            return false;
        }
        *auth_token = android::trim(file_contents);
    } else {
        if (!generate_auth_token(auth_token)) {
            fd.close();
            unlink(path.c_str());
            return false;
        }
        if (!android::write_file_to_end(fd.get(), *auth_token)) {
            E("Oh snap!  couldn't write ~/.emulator_console_auth_token");
            return false;
        }
    }
    return true;
}

// Returns a path to the console auth token file on the current system
String get_auth_token_path_default() {
    System* sys = System::get();
    String result = sys->getHomeDirectory();
    result.append(PATH_SEP);
    result.append(".emulator_console_auth_token");
    return result;
}

fn_get_auth_token_path g_get_auth_token_path = get_auth_token_path_default;

// Gets/creates an auth token in the default path
bool get_auth_token(String* auth_token) {
    return get_auth_token_from(g_get_auth_token_path(), auth_token);
}

}  // namespace console_auth

}  // namespace android

bool android_console_auth_check_authorization_command(const char* line) {
    const char* pos = android::str_skip_white_space_if_any(line);
    if (!android::str_begins_with(pos, "auth")) {
        // didn't find "auth"
        return false;
    }
    pos += 4;

    // skip any white space between "auth" and <auth_token>
    pos = android::str_skip_white_space_if_any(pos);

    String auth_token;
    if (!android::console_auth::get_auth_token(&auth_token)) {
        // couldn't get auth token, disable the console
        return false;
    }

    if (!android::str_begins_with(pos, auth_token.c_str())) {
        return false;
    }
    pos += auth_token.size();
    pos = android::str_skip_white_space_if_any(pos);
    if (pos[0] == 0) {
        return true;
    }
    // there is garbage at the end of the command
    return false;
}

int android_console_auth_get_status() {
    String auth_token;
    if (!android::console_auth::get_auth_token(&auth_token)) {
        // couldn't get auth token, disable the console
        return CONSOLE_AUTH_STATUS_ERROR;
    }

    if (auth_token.size() == 0) {
        // an empty auth token string means that auth isn't required
        return CONSOLE_AUTH_STATUS_DISABLED;
    }
    return CONSOLE_AUTH_STATUS_REQUIRED;
}

void android_console_auth_get_token_path(char* buf, size_t buf_len) {
    String auth_token = android::console_auth::g_get_auth_token_path();
    strncpy(buf, auth_token.c_str(), buf_len);
    buf[buf_len - 1] = 0;
}
