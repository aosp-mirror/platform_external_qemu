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

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace android {
namespace base {

struct MallocStats {
    std::string label;
    int64_t allocated = 0;
    int64_t live = 0;
    int64_t peak = 0;
};

class MemoryUsage {
    DISALLOW_COPY_ASSIGN_AND_MOVE(MemoryUsage);

public:
    static MemoryUsage* get();

    MemoryUsage();
    bool addToGroup(std::string group, std::string func);
    std::string printUsage();
    MallocStats aggregate(std::string label);
    // std::vector<MallocStats> aggregateAll();
    void setEnabled(bool enabled);
    void start();
    void stop();
    class Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace base
}  // namespace android