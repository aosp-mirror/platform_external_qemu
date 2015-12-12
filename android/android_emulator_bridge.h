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

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// Initialize the pipe type "android_emulator_bridge".
// Call this first before doing anything else.
// If the guest is equipped with the associated daemon
// "aebd", and opens a channel, we will allocate buffers for
// android_emulator_bridge and further operations can happen,
// which are detailed below.
void android_emulator_bridge_init(void);
// Only one operation is allowed to run at a time,
// and if any operation fails, we print a message
// and return to an idle state where
// more commands can be issued.

// android_emulator_bridge (aeb) has an interface that exposes
// the commands it can execute. They are:

// speedtest: measure qemu pipe:
// latency
// throughput
void android_emulator_bridge_speedtest();

// screencap: take a screenshot and save it as the specified
// <filename>.
// Relative paths are from the emulator's current
// working directory.
// On error, prints message and returns to idle state, and
// <filename> may be written.
void android_emulator_bridge_screencap(const char* filename);

// shutdown: exit the emulator starting from the guest itself
// (by issuing a reboot -p command).
// The emulator should shut down almost immediately.
// If not, something went wrong with the command
// (no error message is sent)
void android_emulator_bridge_shutdown(void);

// push: transfer a file at <filename> to the guest
// at AEB_DIR/<filename>.
// Relative paths are from the emulator's current
// working directory.
// On error, prints message and returns to idle state.
// AEB_DIR/<filename> on the guest may be written.
void android_emulator_bridge_push(const char* filename);

// pull: transfer a file at <device_filename> on the guest to
// <host_filename> on the host.
// Relative paths are from the emulator's current
// working directory.
// On error, prints message and returns to idle state,
// and <filename> amy be written.
void android_emulator_bridge_pull(
        const char* device_filename,
        const char* host_filename);

// install: transfer a file at <filename> to the guest,
// and additionally install it. This is for APK's.
// Relative paths are from the emulator's current
// working directory.
// On error, prints message and returns to idle state.
// AEB_DIR/<filename> on the guest may be written.
void android_emulator_bridge_install(const char* filename);

// register_monitor: connect android_emulator_bridge to
// some process that serves as a monitor,
// so we can print messages on success/failure.
void android_emulator_bridge_register_monitor(void* mon);
// Using this procedure, which takes a function that follows monitor_printf
// arguments: the monitor (void*), format string (const char* fmt), and format
// arguments.
// If the callback is null, it will use a built-in trivial callback.
void android_emulator_bridge_register_print_callback(
        void (*printf_func)(void* mon, const char* fmt, ...)
);

ANDROID_END_HEADER
