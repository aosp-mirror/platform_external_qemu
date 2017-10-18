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

#include <functional>
#include <map>
#include <vector>

namespace android {
namespace snapshot {

class Hierarchy {
public:

    Hierarchy();

    static Hierarchy* get();

    struct Info {
        std::vector<std::string> roots;
        std::map<std::string,
                 std::vector<std::string> > deps;
    };

    const Info& currentInfo() const;

    DISALLOW_COPY_AND_ASSIGN(Hierarchy);
};

}  // namespace snapshot
}  // namespace android

