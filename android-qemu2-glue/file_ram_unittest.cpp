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

#include "android/base/EintrWrapper.h"
#include "android/base/files/ScopedFd.h"
#include "android/base/misc/FileUtils.h"
#include "android/base/testing/TestTempDir.h"

#include <memory>

using android::base::ScopedFd;
using android::base::StringView;
using android::base::TestTempDir;

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

static constexpr int kTestingPageSize = 4096;
static constexpr int kTesting1Mb = 1048576;

class FileRamTest : public ::testing::Test {
protected:
    void SetUp() override { mTempDir.reset(new TestTempDir("fileramtest")); }

    void TearDown() override { mTempDir.reset(); }

    std::unique_ptr<TestTempDir> mTempDir;
};

TEST_F(FileRamTest, AnonBasic) {
   const int size = kTesting1Mb;

   void* ptr = qemu_ram_mmap(-1 /* fd */, size /* size */, kTestingPageSize /* align */, 0 /* shared */);
   EXPECT_NE(MAP_FAILED, ptr);

   memset(ptr, 0, size);
   qemu_ram_munmap(ptr, size);
}

TEST_F(FileRamTest, FileBasic) {
   std::string tmpFilePath = mTempDir->makeSubPath("ram.img");
   ScopedFd fd(HANDLE_EINTR(open(tmpFilePath.c_str(), O_RDWR | O_CREAT | O_EXCL, 0644)));

   const int size = kTesting1Mb;

   ftruncate(fd.get(), size + kTestingPageSize);

   void* ptr = qemu_ram_mmap(fd.get(), size /* size */, kTestingPageSize /* align */, 0 /* shared */);
   EXPECT_NE(MAP_FAILED, ptr);

   memset(ptr, 0, size);

   qemu_ram_munmap(ptr, size);
}

} // android

