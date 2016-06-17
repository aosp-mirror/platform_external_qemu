// Copyright 20156The Android Open Source Project
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

// Android Console Test Runner==================================================
//
// Introduction
//
// The emulator has a number of virtual devices, such as goldfish_pipe and
// goldfish_audio, that are difficult to test thoroughly without a running guest.
// On top of that, even if a guest is running, it can be hard to run apps
// in just the right way to reproduce a problem at the virtual device level.
//
// The Android Console Test Runner allows one to run arbitrary test functions
// from the emulator console. Once we find a minimal repro case of a
// malfunctioning virtual device, we may write a test function for the
// console test runner, start the emulator and emulator console,
// and run it directly from there, instead of having to always take
// the extra step to do it live in the app.
//
// Usage: Running existing tests and listing available tets
//
// To run a test, issue "test run <name-of-test>" in the emulator console.
// The console will then output "OK" (test passed) or "KO" (test failed).
// To get a list of all tests that are registered, issue "test list."
//
// Usage: Registering new tests
//
// New tests cannot yet be defined through the console. We expect them to be
// compiled along with the rest of the emulator. See test_runner.h for
// information on how to define and register new tests.
// =============================================================================

#pragma once

#include "android/utils/compiler.h"

#include <stdbool.h>

ANDROID_BEGIN_HEADER

typedef struct QAndroidTestAgent {
    // Interface: Mirrors the API in test_runner.h.
    // See test_runner.h for more details.
    //
    // |listTests|: For listing tests that have been registered.
    // Uses double-call convention. See test_runner.h for usage details.
    // Called with "test list" is issued in the console.
    void (*listTests)(char* buf, unsigned int* sz);
    // |addTest|: This is for registering new tests.
    // There is no way to call this from the console yet.
    void (*addTest)(const char* test_id, bool (*func)(void));
    // |runTest|: Run a registered test with ID |test_id|.
    // Called when "test run <name>" is issued in the console.
    bool (*runTest)(const char* test_id);
} QAndroidTestAgent;

ANDROID_END_HEADER
