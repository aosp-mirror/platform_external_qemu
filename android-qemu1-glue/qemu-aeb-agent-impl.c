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

#include "android/android_emulator_bridge.h"
#include "android/emulation/control/aeb_agent.h"

#include "android/utils/debug.h"

static bool shutdown(void) {
    VERBOSE_PRINT(aeb, "aebAgent shutdown (qemu1)");
    android_emulator_bridge_shutdown();
    VERBOSE_PRINT(aeb, "aebAgent shutdown complete (qemu1)");
    return true;
}

static bool push(const char* filename) {
    VERBOSE_PRINT(aeb, "aebAgent push (qemu1)");
    android_emulator_bridge_push(filename);
    VERBOSE_PRINT(aeb, "aebAgent push complete (qemu1)");
    return true;
}

static bool pull(const char* filename) {
    VERBOSE_PRINT(aeb, "aebAgent pull (qemu1)");
    android_emulator_bridge_pull(filename);
    VERBOSE_PRINT(aeb, "aebAgent pull complete (qemu1)");
    return true;
}

static bool install(const char* filename) {
    VERBOSE_PRINT(aeb, "aebAgent install (qemu1)");
    android_emulator_bridge_install(filename);
    VERBOSE_PRINT(aeb, "aebAgent install complete (qemu1)");
    return true;
}

static const QAndroidAEBAgent aebAgent = {
  .shutdown = &shutdown,
  .push = &push,
  .pull = &pull,
  .install = &install
};

const QAndroidAEBAgent* const gQAndroidAEBAgent = &aebAgent;
