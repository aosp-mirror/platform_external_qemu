
// Copyright 2015 The Android Open Source Project
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

#include "android/utils/compiler.h"

#include <stdbool.h>

ANDROID_BEGIN_HEADER

typedef struct QAndroidAEBAgent {
    // AEBAgent: Communicates with Android Emulator Bridge
    // to facilitate installation of large APKs,
    // among other things.

    // On error, messages will be printed.
    // shutdown: turn off the device.
    void (*shutdown)(void);

    // push/pull: transfer files to/from the device.
    // push: files go to AEB_DIR/<filename>
    // pull: files go to <pwd of emulator process/filename>
    void (*push)(const char* filename);
    void (*pull)(const char* device_filename,
            const char* host_filename);

    // transfer an APK file to the device and install it.
    void (*install)(const char* filename);

    // take and download a screenshot
    void (*screencap)(const char* filename);

    // For performance testing qemu pipe
    void (*speedtest)();

    // Stuff to make the command print OK only when the transfer is complete
    void (*register_monitor)(void* mon);
    void (*register_print_callback)(void (*monitor_printf_t)(void*,
                                                             const char*,
                                                             ...));

} QAndroidAEBAgent;

ANDROID_END_HEADER
