// Copyright (C) 2018 The Android Open Source Project
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

#include "android/base/Compiler.h"

#include <inttypes.h>

namespace android {
namespace base {

// cross-platform interface for madvise
enum class MemoryHint {
    DontNeed,
    PageOut,
    Normal,
    Random,
    Sequential,
    Touch,
};

// Returns true if successful, false otherwise.
// |start| must be page-aligned.
bool memoryHint(void* start, uint64_t length, MemoryHint hint);

// Interface to zero out memory and tell the OS to decommit the page,
// but leave it accessible later as well.
// Returns true if successful, false otherwise.
// |start| must be page-aligned.
bool zeroOutMemory(void* start, uint64_t length);

} // namespace base
} // namespace android
