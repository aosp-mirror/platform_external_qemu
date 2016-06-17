// Copyright (C) 2016 The Android Open Source Project
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
// Usage: Running existing tests
//
// To run a test, issue "test run <name-of-test>" in the emulator console.
// The console will then output "OK" (test passed) or "KO" (test failed).
// To get a list of all tests that are registered, issue "test list."
//
// Usage: Registering new tests
//
// This header file provides the entry point test_runner_add_test(id, func).
// |id| is a const char* string that provides the name of the test.
// If a test with the same name already exists, that test will be overwritten.
// |func| is a function of type |android_test_function_t| (bool (void)).
// Tests can be registered from anywhere else in the emulator code
// that includes this header.
//
// =============================================================================

#pragma once

#include "android/utils/compiler.h"

#include <stdbool.h>

ANDROID_BEGIN_HEADER

typedef bool (*android_test_function_t)(void);

void android_test_runner_add_test(const char* id, android_test_function_t func);
bool android_test_runner_run_test(const char* id);
void android_test_runner_list_tests(char* buf, unsigned int* sz);

ANDROID_END_HEADER
