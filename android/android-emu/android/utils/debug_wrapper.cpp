<<<<<<< HEAD   (464e37 Merge "Merge empty history for sparse-5409122-L7540000028739)
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

#include "android/utils/debug.h"

#include "android/base/Log.h"

void base_enable_verbose_logs() {
    VERBOSE_ENABLE(init);
    android::base::setMinLogLevel(android::base::LOG_VERBOSE);
}

void base_disable_verbose_logs() {
    VERBOSE_DISABLE(init);
    android::base::setMinLogLevel(android::base::LOG_INFO);
}
=======
>>>>>>> BRANCH (510a80 Merge "Merge cherrypicks of [1623139] into sparse-7187391-L1)
