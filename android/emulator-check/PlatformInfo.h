// Copyright 2016 The Android Open Source Project
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

#include <string>

namespace android {

// Returns the current window manager. For Mac and Windows, it just returns
// the platform name (Mac/Windows), for linux it queries the installed window
// manager for the name
std::string getWindowManagerName();

// Returns the current desktop environment. For Mac and Windows, it just returns
// the platform name (Mac/Windows), for linux it inspects the common environment
// variables and makes the best guess based on that.
std::string getDesktopEnvironmentName();

}  // namespace android
