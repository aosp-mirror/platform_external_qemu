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

static bool install(const char* args) {
    VERBOSE_PRINT(aeb, "aebAgent install (qemu1)");
    android_emulator_bridge_install(args);
    VERBOSE_PRINT(aeb, "aebAgent install complete (qemu1)");
    return true;
}

static const QAndroidAEBAgent aebAgent = {
  .install = &install
};

const QAndroidAEBAgent* const gQAndroidAEBAgent = &aebAgent;
