// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/snapshot/MemoryWatch.h"

namespace android {
namespace snapshot {

class MemoryAccessWatch::Impl {};

bool MemoryAccessWatch::isSupported() {
    return false;
}

MemoryAccessWatch::MemoryAccessWatch(AccessCallback&&, IdleCallback&&)
    : mImpl(/*new Impl()*/) {}

MemoryAccessWatch::~MemoryAccessWatch() {}

bool MemoryAccessWatch::valid() const {
    return false;
}

bool MemoryAccessWatch::registerMemoryRange(void*, size_t) {
    return false;
}

void MemoryAccessWatch::doneRegistering() {}

bool MemoryAccessWatch::fillPage(void*, size_t, const void*, bool isQuickboot) {
    return false;
}

void MemoryAccessWatch::join() {}

}  // namespace snapshot
}  // namespace android
