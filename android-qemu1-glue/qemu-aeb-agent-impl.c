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
    android_emulator_bridge_shutdown();
    return true;
}

static bool push(const char* filename) {
    android_emulator_bridge_push(filename);
    return true;
}

static bool pull(const char* filename) {
    android_emulator_bridge_pull(filename);
    return true;
}

static bool install(const char* filename) {
    android_emulator_bridge_install(filename);
    return true;
}

static bool screencap(const char* filename) {
    android_emulator_bridge_screencap(filename);
    return true;
}

static void register_monitor(void* mon) {
    android_emulator_bridge_register_monitor(mon);
}

static void register_print_callback(void (*monitor_printf)(void*, const char* ,...)) {
    android_emulator_bridge_register_print_callback(monitor_printf);
}

static const QAndroidAEBAgent aebAgent = {
    .screencap = &screencap,
    .shutdown = &shutdown,
    .push = &push,
    .pull = &pull,
    .install = &install,
    .register_monitor = register_monitor,
    .register_print_callback = register_print_callback
};

const QAndroidAEBAgent* const gQAndroidAEBAgent = &aebAgent;
