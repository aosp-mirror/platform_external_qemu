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

// Android Console Test Runner Agent
// See test_runner.h for documentation on the Test Runner.
// This agent simply passes commands from the console
// to the test runner itself.

#pragma once

#include "android/utils/compiler.h"

#include <stdbool.h>

ANDROID_BEGIN_HEADER

typedef struct QAndroidTestAgent {
    void (*listTests)(char* buf, unsigned int* sz);
    void (*addTest)(const char* test_id, bool (*func)(void));
    bool (*runTest)(const char* test_id);
} QAndroidTestAgent;

ANDROID_END_HEADER
