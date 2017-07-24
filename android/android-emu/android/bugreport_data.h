/* Copyright (C) 2017 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#pragma once
#include "android/base/system/System.h"
#include "android/utils/compiler.h"

#include <string>

using android::base::System;

struct BugreportData {
    std::string androidVer;
    std::string avdDetails;
    std::string cpuModel;
    std::string deviceName;
    std::string emulatorVer;
    std::string hypervisorVer;
    std::string hostOsName;
    std::string reproSteps;
    std::string sdkToolsVer;

    std::string adbLogcatOutput;
    std::string saveLocation;
    std::string adbBugreportFilePath;
    std::string screenshotFilePath;
    std::string bugreportFolderPath;

    bool adbBugreportSucceed = false;
    bool adbLogactSucceed = false;
    bool screenshotSucceed = false;
    bool bugreportReady = false;
    bool bugreportSavedSucceed = false;
    ~BugreportData() {
        System::get()->deleteFile(adbBugreportFilePath);
        System::get()->deleteFile(screenshotFilePath);
    }
};
