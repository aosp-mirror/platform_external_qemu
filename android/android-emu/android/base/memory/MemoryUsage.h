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

#pragma once

#include "android/base/Compiler.h"

#include <memory>
#include <string>

namespace android {
namespace base {

class MemoryUsage {
    DISALLOW_COPY_ASSIGN_AND_MOVE(MemoryUsage);

public:
    static MemoryUsage* get();
    MemoryUsage();
    bool addToGroup(std::string group, std::string func);
    std::string printUsage(int verbosity = 0);
    void start();
    void stop();
private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace base
}  // namespace android