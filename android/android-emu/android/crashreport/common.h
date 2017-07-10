// Copyright 2017 The Android Open Source Project
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

#if defined(__cplusplus) && ! defined(__clang__)
#define ANDROID_NORETURN [[noreturn]]
#else  // !__cplusplus || __clang__
#ifdef _MSC_VER
#define ANDROID_NORETURN __declspec(noreturn)
#else  // !_MSC_VER
#define ANDROID_NORETURN __attribute__((noreturn))
#endif  // !_MSC_VER
#endif  // !__cplusplus || __clang__


