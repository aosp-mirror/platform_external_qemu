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

#ifndef ANDROID_METRICS_DDMS_PING_H
#define ANDROID_METRICS_DDMS_PING_H

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// Run the 'ddms ping' command with appropriate parameters to send usage
// statistics back to Google. Note that this only works if the user agreed
// to it when installing the Android SDK Tools. The emulator itself doesn't
// send anything to the network.
//
// |emulatorVersion| is the emulator's version string to use.
// |glVendor|, |glRenderer| and |glVersion| describe the Host GL driver
// being used by the current session.
void android_ddms_ping(const char* emulatorVersion,
                       const char* glVendor,
                       const char* glRenderer,
                       const char* glVersion);

ANDROID_END_HEADER

#endif  // ANDROID_METRICS_DDMS_PING_H
