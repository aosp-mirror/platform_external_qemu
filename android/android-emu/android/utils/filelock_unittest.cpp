// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/utils/filelock.h"
#include "android/base/system/System.h"

#include <memory>
#include <gtest/gtest.h>

using android::base::System;

TEST(FileLock, DISABLED_testLock) {
    fprintf(stderr, "%s: getting lock\n", __func__);
    FileLock* lock = filelock_create("./asdf.lock");
    fprintf(stderr, "%s: got lock\n", __func__);
    for (int i = 0; i < 5; i++) {
        fprintf(stderr, "%s: %d: wait %d\n", __func__, System::get()->getCurrentProcessId(), i);
        System::get()->sleepMs(1000);
    }
}
