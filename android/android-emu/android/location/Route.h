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

#include "android/base/StringView.h"
#include "route.pb.h"

namespace android {
namespace location {

class Route final {
public:
    Route(const char* name);

    base::StringView name() const { return mName; }
    const emulator_location::RouteMetadata* getProtoInfo();

private:

    bool isValid = false;
    std::string mName;
    emulator_location::RouteMetadata mRoutePb;
    uint64_t mSize = 0; // Size of the protobuf file
};

inline bool operator==(const Route& l, const Route& r) {
    return l.name() == r.name();
}

}  // namespace location
}  // namespace android
