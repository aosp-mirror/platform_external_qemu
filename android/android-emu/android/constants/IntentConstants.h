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

namespace android {
namespace constants {
namespace intent {

constexpr const char* ACTION_BOOT_COMPLETED =
        "android.intent.action.BOOT_COMPLETED";
constexpr const char* ACTION_PACKAGE_ADDED =
        "android.intent.action.PACKAGE_ADDED";
constexpr const char* ACTION_PACKAGE_CHANGED =
        "android.intent.action.PACKAGE_CHANGED";
constexpr const char* ACTION_PACKAGE_REMOVED =
        "android.intent.action.PACKAGE_REMOVED";
constexpr const char* ACTION_PACKAGE_REPLACED =
        "android.intent.action.PACKAGE_REPLACED";
}  // namespace intent
}  // namespace constants
}  // namespace android
