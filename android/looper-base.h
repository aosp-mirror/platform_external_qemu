// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#ifndef ANDROID_LOOPER_BASE_H
#define ANDROID_LOOPER_BASE_H

#include "android/base/async/Looper.h"
#include "android/looper.h"

namespace android {
namespace internal {

typedef ::Looper CLooper;
typedef ::android::base::Looper BaseLooper;

// Implemented by looper-base.cpp, but also used by looper-qemu.cpp
struct GLooper {
    explicit GLooper(BaseLooper* looper);
    ~GLooper();

    CLooper looper;
    BaseLooper* baseLooper;
};

// Super gross hack until we get rid of the stuff in socket_drainer.cpp
BaseLooper* toBaseLooper(CLooper* looper);

}  // namespace internal
}  // namespace android

#endif  // ANDROID_LOOPER_BASE_H
