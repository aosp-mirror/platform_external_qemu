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

#include <unordered_map>

namespace android {
namespace snapshot {

class RamDiff {
public:
    enum class Stage {
        NotLoaded = 0,
        Loaded = 1,
        Touched = 2,
        Changed = 3,
    };

    struct Info {
        void* hva;
        uint64_t gpa;
        Stage stage;
    };

    bool empty = true;
    uint64_t lastIndexFilePos;
    uint32_t pageSize;
    std::unordered_map<void*, Info> diffMap;
};

}  // namespace snapshot
}  // namespace android
