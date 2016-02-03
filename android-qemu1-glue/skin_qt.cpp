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

#include "android-qemu1-glue/skin_qt.h"

#include "android/base/memory/LazyInstance.h"
#include "android/skin/qt/winsys-qt.h"

#include <memory>

static ::android::base::LazyInstance<std::shared_ptr<void>> sInstanceWindow =
        LAZY_INSTANCE_INIT;

extern void skin_acquire_window_inst() {
    sInstanceWindow.get() = skin_winsys_get_shared_ptr();
}

extern void skin_release_window_inst() {
    sInstanceWindow.get().reset();
}
