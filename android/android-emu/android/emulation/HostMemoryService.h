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

enum class HostMemoryServiceCommand {
    None = 0,
    IsSharedRegionAllocated = 1,
    AllocSharedRegion = 2,
    GetHostAddrOfSharedRegion = 3,
    AllocSubRegion = 4,
    FreeSubRegion = 5,
};

enum class CommandStage {
    None = 0,
    GetAddress = 1,
    GetSize = 2,
};

extern void android_host_memory_service_init(void);