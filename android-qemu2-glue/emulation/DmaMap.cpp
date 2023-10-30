// Copyright 2016 The Android Open Source Project
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
#ifdef _MSC_VER
extern "C" {
#include "sysemu/os-win32-msvc.h"
}
#endif
#include "aemu/base/containers/Lookup.h"
#include "aemu/base/logging/Log.h"
#include "android-qemu2-glue/emulation/DmaMap.h"

#include <type_traits>

extern "C" {
#include "qemu/osdep.h"
#include "exec/cpu-common.h"
}  // extern "C"

#include <stdio.h>

namespace qemu2 {

void* DmaMap::doMap(uint64_t addr, uint64_t sz) {
    uint64_t sz_reg = sz;
    void* res = cpu_physical_memory_map(addr, &sz_reg, 1);
    if (sz_reg != sz) {
        derror("DmaMap::doMap(0x0%llx) wanted %llu bytes, got %llu",
                (unsigned long long)addr,
                (unsigned long long)sz,
                (unsigned long long)sz_reg);
        if (res) {
            cpu_physical_memory_unmap(res, sz_reg, 1, sz_reg);
        }
        return nullptr;
    }
    return res;
}

void DmaMap::doUnmap(void* mapped, uint64_t sz) {
    cpu_physical_memory_unmap(mapped, sz, 1, sz);
}

}  // namespace qemu2
