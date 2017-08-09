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

#include "android/base/StringView.h"
#include <string>

namespace android {
namespace snapshot {

class Snapshot final {
public:
    Snapshot(const char* name);

    base::StringView name() const { return mName; }
    base::StringView dataDir() const { return mDataDir; }

    bool save();
    bool load();

private:
    std::string mName;
    std::string mDataDir;
};

inline bool operator==(const Snapshot& l, const Snapshot& r) {
    return l.name() == r.name();
}

}  // namespace snapshot
}  // namespace android
