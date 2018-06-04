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

#include <gtest/gtest.h>

extern "C" {
#include "qemu/osdep.h"
#include "qemu-common.h"
#include "qom/object.h"
#include "qemu/mmap-alloc.h"
}

#ifdef _WIN32
#define MAP_FAILED 0
#endif

namespace android {

TEST(FileRamTest, Basic) {
   void* ptr = qemu_ram_mmap(-1 /* fd */, 0 /* size */, 0 /* align */, 0 /* shared */); 
   EXPECT_EQ(MAP_FAILED, ptr);
}

} // android

