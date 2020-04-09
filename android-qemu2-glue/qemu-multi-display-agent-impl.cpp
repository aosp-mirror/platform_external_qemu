// Copyright 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android-qemu2-glue/qemu-control-impl.h"
#include "android/emulation/control/multi_display_agent.h"
#include "android/emulation/MultiDisplayPipe.h"
#include "android/opengles.h"

using android::MultiDisplayPipe;

static const QAndroidMultiDisplayAgent sMultiDisplayAgent = {
        .setMultiDisplay = [](uint32_t id,
                              int32_t x,
                              int32_t y,
                              uint32_t w,
                              uint32_t h,
                              uint32_t dpi,
                              uint32_t flag,
                              bool add) {
            MultiDisplayPipe::setMultiDisplay(id, x, y, w, h, dpi, flag, add);
        },
        .tryLockMultiDisplayOnLoad = [](void)->bool {
            return android_tryLockMultiDisplayOnLoad();
        },
        .unlockMultiDisplayOnLoad = [](void) {
            android_unlockMultiDisplayOnLoad();
        }
};

const QAndroidMultiDisplayAgent* const gQAndroidMultiDisplayAgent = &sMultiDisplayAgent;
