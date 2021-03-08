// Copyright (C) 2015 The Android Open Source Project
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

#include "android/opengl/GpuFrameBridge.h"

#include "android/base/async/Looper.h"
#include "android/base/Log.h"
#include "android/base/memory/ScopedPtr.h"

#include <gtest/gtest.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

namespace android {
namespace opengl {

using android::base::ScopedPtr;
using android::base::Looper;

TEST(GpuFrameBridge, postFrameWithinSingleThread) {
    GpuFrameBridge* bridge =
            GpuFrameBridge::create();
    EXPECT_TRUE(bridge);

    static const unsigned char kFrame0[4] = {
        0xff, 0x80, 0x40, 0xff,
    };

    bridge->postRecordFrame(1, 1, kFrame0);
    unsigned char* pixel = (unsigned char*)bridge->getRecordFrame();
    for (size_t n = 0; n < sizeof(kFrame0); ++n) {
        EXPECT_EQ(kFrame0[n], pixel[n]) << "# " << n;
    }
}

}  // namespace opengl
}  // namespace android
