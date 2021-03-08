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

#pragma once

#include "android/base/Compiler.h"
#include "android/base/containers/SmallVector.h"
#include "android/base/files/Stream.h"

namespace android {
namespace base {

class CompressingStream : public Stream {
    DISALLOW_COPY_AND_ASSIGN(CompressingStream);

public:
    CompressingStream(Stream& output);
    ~CompressingStream();

    ssize_t read(void* buffer, size_t size) override;
    ssize_t write(const void* buffer, size_t size) override;

private:
    Stream& mOutput;
    void* mLzStream;
    SmallFixedVector<char, 512> mBuffer;
};

}  // namespace base
}  // namespace android
