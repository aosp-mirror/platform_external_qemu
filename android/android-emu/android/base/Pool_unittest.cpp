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
#include "android/base/Pool.h"

#include <gtest/gtest.h>

#include <string>

namespace android {
namespace base {

// Tests basic function of pool:
// 1. Can allocate objects
// 2. Can acquire their pointer
// 3. Buffers preserved after deleting other objects
TEST(Pool, Basic) {
    Pool pool(8, 4096);
}


} // namespace base
} // namespace android

