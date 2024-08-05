// Copyright (C) 2023 The Android Open Source Project
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

#pragma once

#include "android/avd/avd-export.h"
#include "android/avd/info.h"
#include "android/avd/keys.h"
#include "android/avd/util.h"
#include "android/utils/compiler.h"
#include "android/utils/file_data.h"
#include "android/utils/ini.h"
#include "host-common/hw-config.h"

ANDROID_BEGIN_HEADER
struct AvdInfo {
    /* for the Android build system case */
    char inAndroidBuild;
    char* androidOut;
    char* androidBuildRoot;
    char* targetArch;
    char* targetAbi;
    char* acpiIniPath;
    char* target;  // The target string in rootIni.

    /* for the normal virtual device case */
    char* deviceName;
    char* deviceId;
    char* sdkRootPath;
    char* searchPaths[MAX_SEARCH_PATHS];
    int numSearchPaths;
    char* contentPath;
    char* rootIniPath;
    CIniFile* rootIni;   /* root <foo>.ini file, empty if missing */
    CIniFile* configIni; /* virtual device's config.ini, NULL if missing */
    CIniFile* skinHardwareIni; /* skin-specific hardware.ini */

    /* for both */
    int apiLevel;
    int incrementalVersion;

    /* For preview releases where we don't know the exact API level this flag
     * indicates that at least we know it's M+ (for some code that needs to
     * select either legacy or modern operation mode.
     */
    bool isMarshmallowOrHigher;
    bool isGoogleApis;
    bool isUserBuild;
    bool isAtd;
    AvdFlavor flavor;
    char* skinName;            /* skin name */
    char* skinDirPath;         /* skin directory */
    char* coreHardwareIniPath; /* core hardware.ini path */
    char* snapshotLockPath;    /* core snapshot.lock path */
    char* multiInstanceLockPath;

    FileData buildProperties[1]; /* build.prop file */
    FileData bootProperties[1];  /* boot.prop file */

    /* image files */
    char* imagePath[AVD_IMAGE_MAX];
    char imageState[AVD_IMAGE_MAX];

    /* skip checks */
    bool noChecks;
    const char* sysdir;
};
ANDROID_END_HEADER