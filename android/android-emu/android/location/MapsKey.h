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
    virtual const char* userMapsKey() const;
    virtual const char* androidStudioMapsKey() const;
    virtual void setUserMapsKey(const base::StringView& key);
    virtual void setAndroidStudioMapsKey(const base::StringView& key);
    static MapsKey* get();

protected:
    MapsKey() = default;
};

}  // namespace location
}  // namespace android
