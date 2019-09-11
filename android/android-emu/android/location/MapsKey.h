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

#include "android/base/StringView.h"

#include <string>

namespace android {
namespace location {

class MapsKey {
public:
    virtual ~MapsKey() = default;
    // Key provided by the command-line flag
    virtual const char* userMapsKey() const = 0;
    virtual const char* androidStudioMapsKey() const = 0;
    virtual void setUserMapsKey(const base::StringView& key) = 0;
    virtual void setAndroidStudioMapsKey(const base::StringView& key) = 0;
    static MapsKey* get();

protected:
    MapsKey() = default;
};

}  // namespace location
}  // namespace android
