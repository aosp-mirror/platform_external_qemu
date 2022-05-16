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
#include <stdint.h>

ANDROID_BEGIN_HEADER

typedef struct QAndroidNetAgent {
    bool (*isSlirpInited)();
    bool (*slirpRedir)(bool isUdp, int hostPort, uint32_t guestAddr, int guestPort);
    bool (*slirpUnredir)(bool isUdp, int hostPort);
    bool (*slirpBlockSsid)(const char* ssid, bool enable);
} QAndroidNetAgent;

ANDROID_END_HEADER
