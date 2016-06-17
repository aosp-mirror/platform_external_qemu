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

typedef void (*test_function_t)(void);

#ifdef __cplusplus
extern "C" {
#endif

void test_runner_init(void);
void test_runner_add_test(const char* id, test_function_t func);
bool test_runner_run_test(const char* id);

#ifdef __cplusplus
}
#endif

ANDROID_END_HEADER
