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

#include "android/error-messages.h"

#include <string>

extern "C" {

// Keep track of if an error was actually set, this way we don't have to encode
// this state using a magic number in the actual error code.
static bool init_error_occurred = false;
static int init_error_code = 0;
static std::string init_error_message;

void android_init_error_set(int error_code, const char* error_message) {
    init_error_occurred = true;
    init_error_code = error_code;
    init_error_message = error_message ? error_message : kUnknownInitError;
}

bool android_init_error_occurred(void) {
    return init_error_occurred;
}

void android_init_error_clear(void) {
    init_error_occurred = false;
    init_error_code = 0;
    init_error_message.clear();
}

int android_init_error_get_error_code(void) {
    return init_error_code;
}

const char* android_init_error_get_message(void) {
    if (init_error_message.empty()) {
        return nullptr;
    }
    return init_error_message.c_str();
}

const char* const kHaxVcpuSyncFailed =
        "Unfortunately, VirtualBox 4.3.30+ does not allow multiple hypervisors "
        "to co-exist.  In order for VirtualBox and the Android Emulator to "
        "co-exist, VirtualBox must change back to shared use.  Please ask "
        "VirtualBox to consider this change here: "
        "https://www.virtualbox.org/ticket/14294";

const char* const kUnknownInitError =
        "An unkown error occured when starting Android Emulator. Please "
        "consider filing a bug report describing what happened";

const char* const kNotEnoughMemForGuestError =
        "Android Emulator could not allocate %.1f GB "
        "of memory for the current AVD configuration. "
        "Consider adjusting the RAM size of your AVD "
        "in the AVD Manager.\n\n"
        "Error detail: QEMU '%s'";

}  // extern "C"
