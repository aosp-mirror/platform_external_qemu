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

#include "android/base/Version.h"
#include "android/base/String.h"

namespace android {

struct StudioHelper {
    // Extract the Android Studio version from directory name |dirName|.
    // dirName is expected to match the following pattern
    //            [.]AndroidStudio[Preview]X.Y
    // where X,Y represent a major/minor revision number.
    // If |dirName| is not formatted following the AndroidStudio
    // preferences/configuration directory name pattern, an invalid
    // Version will be returned.
    //
    static const base::Version extractAndroidStudioVersion(
            const char* const dirName);

    // Scan path |scanPath| for AndroidStudio preferences/configuration
    // directries and return the path to the latest one, or NULL
    // if none can be found. Returned value is a String
    //
    static base::String latestAndroidStudioDir(const base::String& scanPath);

    // Construct and return the complete path to Android Studio XML
    // preferences |filename| under path |studioPath|.
    // Returned value is a String
    static base::String pathToStudioXML(const base::String& studioPath,
                                        const base::String& filename);

#ifdef _WIN32
    // Microsoft Windows only
    // Construct and return the complete path to Android Studio
    // UUID storage file. Returned value is a String
    //
    static android::base::String pathToStudioUUIDWindows();
#endif
};

}  // namespace android
