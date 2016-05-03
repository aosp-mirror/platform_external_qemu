// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/misc/Random.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <wincrypt.h>
#endif

#include <stdio.h>
#include <stdlib.h>

namespace android {

void genRandomBytes(uint8_t* buffer, size_t bufferLen) {
    if (!bufferLen) {
        return;
    }
    bool failure = true;
#ifdef _WIN32
    HCRYPTPROV cryptProvider = 0;
    if (::CryptAcquireContext(&cryptProvider, NULL, NULL, PROV_RSA_FULL,
                              CRYPT_VERIFYCONTEXT | CRYPT_SILENT)) {
        failure = !::CryptGenRandom(cryptProvider, bufferLen, buffer);
        ::CryptReleaseContext(cryptProvider, 0U);
    }
#else   // !_WIN32
    {
        FILE* file = fopen("/dev/random", "rb");
        if (file) {
            failure = (fread(buffer, bufferLen, 1, file) == 1);
            fclose(file);
        }
    }
#endif  // !_WIN32
    if (failure) {
        // Backup scheme: use rand() to initialize individual values.
        for (size_t n = 0; n < bufferLen; ++n) {
            buffer[n] = static_cast<uint8_t>(rand());
        }
    }
}

}  // namespace android
