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


#pragma once

#include "android/utils/compiler.h"

#include <stdbool.h>

ANDROID_BEGIN_HEADER

// |android_test_function_t| is the function type
// that represents an Android Test Runner test.
// Each such function does not take any arguments,
// and returns true on success and false on failure.
typedef bool (*android_test_function_t)(void);

// |android_test_runner_add_test| is an entry point for registering
// new Android Console Test Runner tests.
//
// |id| is a const char* string that provides the name of the test.
// If a test with the same name already exists, that test will be overwritten.
// Tests can be registered from anywhere else in the emulator code
// that includes this header.
//
void android_test_runner_add_test(const char* id, android_test_function_t func);

// |android_test_runner_run_test| attempts to run the test with |id|.
// Returns true if the test was able to run, and false otherwise,
// such as when the test id |id| does not exist in the set of
// registered tests.
//
// The test result itself is appended to a file whose location
// can be obtained using android_test_runner_get_test_result_location.
bool android_test_runner_run_test(const char* id);

// |android_test_runner_list_tests| outputs a string into |buf|
// that lists all the available tests. The name of each test
// is wrapped in square brackets and there is a newline after each entry.
//
// To use this function when the number of tests
// (and thus maximum string size) is unknown, we use a convention
// where there are two calls involved.
//
// First, call  with |buf| = NULL and non-null |sz|, which
// stores the number of bytes needed in |sz|.
//
// Then, allocate a char* buffer of the proper size
// and use it as the |buf| argument for the second call.
//
// After that second call, buf should hold the string of tests.
void android_test_runner_list_tests(char* buf, unsigned int* sz);

// |android_test_runner_get_result_location| outputs the filename
// of where the test results are stored in buf. It supports a similar protocol
// as android_test_runner_list_tests, with two calls if one wishes
// the size of |buf| to be dynamically determined.
void android_test_runner_get_result_location(char* buf, unsigned int* sz);

ANDROID_END_HEADER
