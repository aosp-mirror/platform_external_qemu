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
#include "point.pb.h"

namespace android {
namespace location {

class Point final {
public:
    Point(const char* name);

    base::StringView name() const { return mName; }
    const emulator_location::PointMetadata* getProtoInfo();

private:

    bool isValid = false;
    std::string mName;
    emulator_location::PointMetadata mPointPb;
    uint64_t mSize = 0; // Size of the protobuf file
};

inline bool operator==(const Point& l, const Point& r) {
    return l.name() == r.name();
}

}  // namespace location
}  // namespace android
