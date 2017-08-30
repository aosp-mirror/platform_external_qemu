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

#include "android/emulation/Keymaster3.h"
#include "android/emulation/AndroidMessagePipe.h"
#include "android/utils/debug.h"
#include "keymaster_ipc.h"

#include <assert.h>

#include <memory>
#include <vector>

// #define DEBUG 1

#ifdef DEBUG
#include <stdio.h>
#define D(...)                                     \
    do {                                           \
        printf("%s:%d: ", __FUNCTION__, __LINE__); \
        printf(__VA_ARGS__);                       \
        fflush(stdout);                            \
    } while (0)
#else
#define D(...) ((void)0)
#endif

namespace android {

void keymasterDecodeAndExecute(const std::vector<uint8_t>& input,
                               std::vector<uint8_t>* output) {
    keymaster_ipc_call(input, output);
}

void registerKeymaster3PipeService() {
    android::AndroidPipe::Service::add(new android::AndroidMessagePipe::Service(
            "KeymasterService3", keymasterDecodeAndExecute));
}
}  // namespace android

extern "C" void android_init_keymaster3(void) {
    keymaster_ipc_init();
    android::registerKeymaster3PipeService();
}
