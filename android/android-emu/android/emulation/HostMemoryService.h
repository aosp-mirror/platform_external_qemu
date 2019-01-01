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

#include <inttypes.h>

enum class HostMemoryServiceCommand {
    NoCommand = 0,
    IsSharedRegionAllocated = 1,
    AllocSharedRegion = 2,
    GetHostAddrOfSharedRegion = 3,
    AllocSubRegion = 4,
    FreeSubRegion = 5,
    SetBaseOffsetOfSharedRegion = 6,
};

extern void android_host_memory_service_init(void);

typedef void* (*host_memory_ptr_get_t)(uint64_t);

extern void* android_host_memory_service_get_ptr_from_guest_offset(uint64_t offset);
