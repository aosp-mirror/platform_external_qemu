// Copyright (C) 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"

namespace android {
namespace videoplayer {

class VideoPlayer;
class PacketQueue;

struct VideoPlayerWaitInfo {
    android::base::Lock lock;  // protects other parts of this struct
    bool done = false;
    android::base::ConditionVariable cvDone;
};

}  // namespace videoplayer
}  // namespace android
