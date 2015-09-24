// Copyright 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "android/base/String.h"

namespace android {

// Helper struct to hold static methods related to misc. configuration
// directories used by the emulator.
struct ConfigDirs {
    // Return the user-specific directory containing Android-related
    // configuration files. The default location can be overriden:
    //
    // - If ANDROID_EMULATOR_HOME is defined in the environment, its
    //   value is returned by this function.
    //
    // - Otherwise, if ANDROID_SDK_HOME is defined in the environment,
    //   this function returns one of its sub-directories.
    //
    // - Otherwise, this returns a subdirectory of the user's home dir.
    static android::base::String getUserDirectory();

    // Return the root path containing all AVD sub-directories.
    // More specifically:
    //
    // - If ANDROID_AVD_HOME is defined in the environment, its value is
    //   returned.
    //
    // - Otherwise, a sub-directory named 'avd' of getUserDirectory()'s
    //   output is returned.
    static android::base::String getAvdRootDirectory();
};

}  // namespace android
