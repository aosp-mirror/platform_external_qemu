// Copyright 2018 The Android Open Source Project
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

#include "android/base/containers/Lookup.h"

#include "android-qemu2-glue/emulation/HostmemAlloc.h"

#include <type_traits>

extern "C" {
#include "qemu/osdep.h"
#include "exec/cpu-common.h"
}  // extern "C"

#include <stdio.h>

namespace qemu2 {

bool HostmemAlloc::addRegion(uint64_t gpa, void* hva, uint64_t size) {
    return true;
}

bool HostmemAlloc::removeRegion(uint64_t gpa, uint64_t size) {
    return true;
}

}  // namespace qemu2
