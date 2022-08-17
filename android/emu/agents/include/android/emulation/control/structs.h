// Copyright 2020 The Android Open Source Project
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

#include <inttypes.h>

ANDROID_BEGIN_HEADER
// This file defines common structs used by the interfaces defined for
// AndroidEmu <--> QEMU interaction.

// A struct describing the information about host memory associated
// with a host memory id. Used with virtio-gpu-next.
struct HostmemEntry {
    uint64_t id;
    void* hva;
    uint64_t size;
    uint32_t caching;
};

// Called by hostmemRegister(..)
struct MemEntry {
    void* hva;
    uint64_t size;
    uint32_t register_fixed;
    uint64_t fixed_id;
    uint32_t caching;
};
ANDROID_END_HEADER
