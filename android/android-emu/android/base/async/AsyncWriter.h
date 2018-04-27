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

#pragma once

#include "android/base/async/AsyncStatus.h"
#include "android/base/async/Looper.h"

#include <cstdint>

namespace android {
namespace base {

class AsyncWriter {
public:
    AsyncWriter() = default;

    void reset(const void* buffer, size_t bufferSize, Looper::FdWatch* watch);

    AsyncStatus run();

private:
    const uint8_t* mBuffer{nullptr};
    size_t mBufferSize{0U};
    size_t mPos{0U};
    Looper::FdWatch* mFdWatch{nullptr};
};

}  // namespace base
}  // namespace android
