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

#include <functional>

namespace android {
namespace location {

class StudioMapsKey {
public:
    // Called when the query for the maps file is complete. The path to the
    // file is returned and an opaque pointer. If the file does not exist,
    // the path will be empty.
    using UpdateCallback = std::function<void(base::StringView, void*)>;

    // Creating an instance will run a query in a background thread to
    // fetch the maps key. |cb| will be called once the query is complete.
    // Use getFilePath() once the query completes to get the path to the file.
    static StudioMapsKey* create(UpdateCallback cb, void* opaque);

protected:
    StudioMapsKey() = default;
};  // StudioMapsKey

}  // namespace location
}  // namespace android
