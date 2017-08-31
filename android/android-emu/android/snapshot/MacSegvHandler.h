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

class MacSegvHandler {
public:
    using BadAccessCallback = void (*)(void* faultvaddr);
    MacSegvHandler(BadAccessCallback f);
    ~MacSegvHandler();

    void registerMemoryRange(void* start, uint64_t size);
    bool isRegistered(void* ptr);

private:
    using MemoryRange = std::pair<char*, uint64_t>;
    using MemoryRanges = std::vector<MemoryRange>;
    
    void clearRegistered();

    MemoryRanges mRanges = {};
};
