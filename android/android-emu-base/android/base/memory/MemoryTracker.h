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

#include <atomic>
#include <memory>
#include <string>

/*Example usage:
#include "GLcommon/GLESmacros.h"
// inside function where you want to trace
MEM_TRACE("<Group name>")

Implementation:
The tracker registers hooks to tcmalloc allocator for
functions explicitly declared as being tracked. Now, we tracker every APIs in
emugl translator library as "EMUGL".  When the hooks are
invoked, the callback function gets the backtraces as a list of program
counters. Then, it walks up the stack and checks if any registerd
function is used in this malloc/free operation. Once hit, it will record
stats for total allocation and live memory for each individual function.

*/
namespace android {
namespace base {

class MemoryTracker {
    DISALLOW_COPY_ASSIGN_AND_MOVE(MemoryTracker);

public:
    struct MallocStats {
        std::atomic<int64_t> mAllocated{0};
        std::atomic<int64_t> mLive{0};
        std::atomic<int64_t> mPeak{0};
    };

    static MemoryTracker* get();
    MemoryTracker();
    bool addToGroup(const std::string& group, const std::string& func);
    std::string printUsage(int verbosity = 0);
    void start();
    void stop();
    bool isEnabled();
    std::unique_ptr<MemoryTracker::MallocStats> getUsage(
            const std::string& group);

private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace base
}  // namespace android