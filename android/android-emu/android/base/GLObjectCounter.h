// Copyright 2018 The Android Open Source Project
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

#include <memory>
#include <string>
#include <vector>

namespace android {
namespace base {
class GLObjectCounter {
    DISALLOW_COPY_ASSIGN_AND_MOVE(GLObjectCounter);

public:
    GLObjectCounter();
    static GLObjectCounter* get();
    void incCount(size_t type);
    void decCount(size_t type);
    std::vector<size_t> getCounts();
    std::string printUsage();

private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
};
}  // namespace base
}  // namespace android
