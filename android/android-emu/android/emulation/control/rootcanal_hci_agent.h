// Copyright (C) 2021 The Android Open Source Project
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
#include <stdbool.h>
#include <sys/types.h>
#include "android/utils/compiler.h"

#ifdef _MSC_VER
#include "msvc-posix.h"
#endif

ANDROID_BEGIN_HEADER

typedef void (*dataAvailableCallback)(void* opaque);


// An agent that talks to hci vsock in android.
// This can be used to send and receive data packets from a system image
// that has a virtual hci enabled.
typedef struct QAndroidHciAgent {
    // Fill the buffer of the given size with bytes.
    // returns the number of bytes available, or -1 when no data is available.
    // errno will be set to EAGAIN when no data is available.
    ssize_t (*recv)(uint8_t* buffer, uint64_t bufferSize);

    // Sends the given set of bytes to the virtual hci driver in the guest.
    // all bytes are always send.
    ssize_t (*send)(const uint8_t* buffer, uint64_t bufferSize);

    // Amount of bytes that are currently available for reading.
    size_t (*available)();

    // Callback that will be invoked when there a bytes available for reading.
    void (*registerDataAvailableCallback)(void* opaque, dataAvailableCallback callback);
} QAndroidHciAgent;

ANDROID_END_HEADER
