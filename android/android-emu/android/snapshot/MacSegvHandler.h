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

#include <memory>
#include <vector>

#include <inttypes.h>
#include <stdbool.h>

// MacSegvHandler is a class that sets up Mac-specific signal handling
// using Mach exceptions and handles them, including forwarding real crashes
// to the crash handler (if attached) or directly to the OS.
class MacSegvHandler {
public:
    using BadAccessCallback = void (*)(void* faultvaddr);

    // Constructor will also initialize and install a new Mach
    // exception handler for EXC_BAD_ACCESS globally for this task.
    MacSegvHandler(BadAccessCallback f);
    // Destructor restores the previous Mach exception handler for
    // this task.
    ~MacSegvHandler();

    // registerMemoryRange() needs to be called in order for
    // MacSegvHandler to be able to tell which memory accesses
    // are for RAM loading and which are actual crashes that
    // we need to pass through.
    void registerMemoryRange(void* start, uint64_t size);

    // Queries whether |ptr| is for RAM loading or is an illegal
    // access that should be passed through.
    bool isRegistered(void* ptr) const;

private:
    using MemoryRange = std::pair<char*, uint64_t>;
    using MemoryRanges = std::vector<MemoryRange>;

    void clearRegistered();

    MemoryRanges mRanges = {};
};
