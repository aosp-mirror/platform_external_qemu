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

#include "android/bugreport.h"

#include "android/avd/keys.h"
#include "android/base/StringFormat.h"
#include "android/base/files/PathUtils.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/system/System.h"
#include "android/bugreport_data.h"
#include "android/emulation/ComponentVersion.h"
#include "android/emulation/ConfigDirs.h"
#include "android/emulation/CpuAccelerator.h"
#include "android/emulation/control/AdbBugReportServices.h"
#include "android/emulation/control/ScreenCapturer.h"
#include "android/emulator-window.h"
#include "android/globals.h"
#include "android/update-check/VersionExtractor.h"
#include "android/utils/path.h"

#include <algorithm>
#include <fstream>
#include <vector>

using android::base::LazyInstance;
using android::base::PathUtils;
using android::base::StringFormat;
using android::base::StringView;
using android::base::System;
using android::base::Version;
using android::emulation::AdbBugReportServices;
using android::emulation::AdbInterface;
using android::emulation::ScreenCapturer;

class Bugreport {
public:
    void initialize(AdbInterface* adb, ScreenCapturer* screenCapturer) {
        // In the case where screenCapturer is nullptr, screen capturer
        // service is disabled by default.
        mData.reset(new BugreportData());
        mBugReportServices.reset(new AdbBugReportServices(adb));
        if (screenCapturer) {
            const int MIN_SCREENSHOT_API = 14;
            const int DEFAULT_API_LEVEL = 1000;
            int apiLevel = avdInfo_getApiLevel(android_avdInfo);
            if (apiLevel != DEFAULT_API_LEVEL &&
                apiLevel >= MIN_SCREENSHOT_API) {
                mScreenCapturer = screenCapturer;
            }
        }
    };

    BugreportData* generateBugreport(BugreportCallback callback,
                                     void* context) {
        mData->bugreportReady = false;
        if (mFirstTime) {
            android::update_check::VersionExtractor vEx;
            Version curEmuVersion = vEx.getCurrentVersion();
            mData->emulatorVer = curEmuVersion.isValid()
                                         ? curEmuVersion.toString()
                                         : "Unknown";
            android::CpuAccelerator accel = android::GetCurrentCpuAccelerator();
            Version accelVersion = android::GetCurrentCpuAcceleratorVersion();
            if (accelVersion.isValid()) {
                mData->hypervisorVer = StringFormat(
                        "%s %s", android::CpuAcceleratorToString(accel),
                        accelVersion.toString());
            }

            char versionString[128];
            int apiLevel = avdInfo_getApiLevel(android_avdInfo);
            avdInfo_getFullApiName(apiLevel, versionString, 128);
            mData->androidVer = std::string(versionString);

            const char* deviceName = avdInfo_getName(android_avdInfo);
            mData->deviceName = std::string(deviceName);

            mData->hostOsName = System::get()->getOsName();

            mData->sdkToolsVer =
                    android::getCurrentSdkVersion(
                            android::ConfigDirs::getSdkRootDirectory(),
                            android::SdkComponentType::Tools)
                            .toString();

            mData->cpuModel = android::GetCpuInfo().second;

            loadAvdDetails();
            mFirstTime = false;
            if (callback) {
                callback(mData.get(), BugreportDataType::AvdDetails, context);
            }
        }

        if (!mBugReportServices->isLogcatInFlight()) {
            mBugReportServices->generateAdbLogcatInMemory(
                    [this, callback, context](
                            AdbBugReportServices::Result result,
                            StringView output) {
                        if (result == AdbBugReportServices::Result::Success) {
                            mData->adbLogactSucceed = true;
                            mData->adbLogcatOutput = output;
                        } else {
                            mData->adbLogactSucceed = false;
                        }
                        if (callback) {
                            callback(mData.get(), BugreportDataType::AdbLogcat,
                                     context);
                        }
                    });
        }

        if (mScreenCapturer && !mScreenCapturer->isInFlight()) {
            mScreenCapturer->capture(
                    System::get()->getTempDir(),
                    [this, callback, context](ScreenCapturer::Result result,
                                              StringView filePath) {
                        if (System::get()->deleteFile(
                                    mData->screenshotFilePath)) {
                            mData->screenshotFilePath.clear();
                        }
                        if (result == ScreenCapturer::Result::kSuccess &&
                            System::get()->pathIsFile(filePath) &&
                            System::get()->pathCanRead(filePath)) {
                            mData->screenshotSucceed = true;
                            mData->screenshotFilePath = filePath;
                        }
                        if (callback) {
                            callback(mData.get(), BugreportDataType::Screenshot,
                                     context);
                        }
                    });
        }

        if (!mBugReportServices->isBugReportInFlight()) {
            mBugReportServices->generateBugReport(
                    System::get()->getTempDir(),
                    [this, callback, context](
                            AdbBugReportServices::Result result,
                            StringView filePath) {
                        if (System::get()->deleteFile(
                                    mData->adbBugreportFilePath)) {
                            mData->adbBugreportFilePath.clear();
                        }
                        if (result == AdbBugReportServices::Result::Success &&
                            System::get()->pathIsFile(filePath) &&
                            System::get()->pathCanRead(filePath)) {
                            mData->adbBugreportSucceed = true;
                            mData->adbBugreportFilePath = filePath;
                        }
                        mData->bugreportReady = true;
                        if (callback) {
                            callback(mData.get(),
                                     BugreportDataType::AdbBugreport, context);
                        }
                    });
        }

        return mData.get();
    };

    bool saveBugreport(BugreportData* data, int savingFlag) {
        if (data->bugreportSavedSucceed)
            return true;

        if (data->saveLocation.empty()) {
            // Try desktop directory first, then fall back to
            // home directory
            std::string homeDir = System::get()->getHomeDirectory();
            std::string desktopDir = PathUtils::join(homeDir, "Desktop");
            if (!setSavePath(data, desktopDir.c_str()) &&
                !setSavePath(data, homeDir.c_str())) {
                return false;
            }
        }

        data->bugreportFolderPath = PathUtils::join(
                data->saveLocation,
                AdbBugReportServices::generateUniqueBugreportName());

        if (path_mkdir_if_needed(data->bugreportFolderPath.c_str(), 0755)) {
            return false;
        }

        if (savingFlag & BugreportDataType::AdbBugreport &&
            !data->adbBugreportFilePath.empty()) {
            std::string bugreportBaseName;
            if (PathUtils::extension(data->adbBugreportFilePath) == ".zip") {
                bugreportBaseName = "bugreport.zip";
            } else {
                bugreportBaseName = "bugreport.txt";
            }
            path_copy_file(PathUtils::join(data->bugreportFolderPath,
                                           bugreportBaseName)
                                   .c_str(),
                           data->adbBugreportFilePath.c_str());
        }

        if (savingFlag & BugreportDataType::Screenshot &&
            !data->screenshotFilePath.empty()) {
            std::string screenshotBaseName = "screenshot.png";
            path_copy_file(PathUtils::join(data->bugreportFolderPath,
                                           screenshotBaseName)
                                   .c_str(),
                           data->screenshotFilePath.c_str());
        }

        if (savingFlag & BugreportDataType::AvdDetails &&
            !data->avdDetails.empty()) {
            auto avdDetailsFilePath = PathUtils::join(data->bugreportFolderPath,
                                                      "avd_details.txt");
            std::ofstream outFile(avdDetailsFilePath.c_str(),
                                  std::ios_base::out | std::ios_base::trunc);
            outFile << data->avdDetails << std::endl;
        }

        if (!data->reproSteps.empty()) {
            auto reproStepsFilePath = PathUtils::join(data->bugreportFolderPath,
                                                      "repro_steps.txt");
            std::ofstream outFile(reproStepsFilePath.c_str(),
                                  std::ios_base::out | std::ios_base::trunc);
            outFile << data->reproSteps << std::endl;
        }
        data->bugreportSavedSucceed = true;
        return true;
    };

    void setReproSteps(BugreportData* data, const char* reproSteps) {
        data->reproSteps = reproSteps;
    };

    bool setSavePath(BugreportData* data, const char* savePath) {
        if (System::get()->pathIsDir(savePath) &&
            System::get()->pathCanWrite(savePath)) {
            data->saveLocation = savePath;
            return true;
        } else {
            return false;
        }
    };

private:
    void loadAvdDetails() {
        std::vector<const char*> avdDetailsNoDisplayKeys{
                ABI_TYPE,    CPU_ARCH,    SKIN_NAME, SKIN_PATH,
                SDCARD_SIZE, SDCARD_PATH, IMAGES_2};

        CIniFile* configIni = avdInfo_getConfigIni(android_avdInfo);

        if (configIni) {
            mData->avdDetails.append(
                    StringFormat("Name: %s\n", mData->deviceName));
            mData->avdDetails.append(
                    StringFormat("CPU/ABI: %s\n",
                                 avdInfo_getTargetCpuArch(android_avdInfo)));
            mData->avdDetails.append(StringFormat(
                    "Path: %s\n", avdInfo_getContentPath(android_avdInfo)));
            mData->avdDetails.append(
                    StringFormat("Target: %s (API level %d)\n",
                                 avdInfo_getTag(android_avdInfo),
                                 avdInfo_getApiLevel(android_avdInfo)));
            char* skinName;
            char* skinDir;
            avdInfo_getSkinInfo(android_avdInfo, &skinName, &skinDir);
            if (skinName)
                mData->avdDetails.append(StringFormat("Skin: %s\n", skinName));

            const char* sdcard = avdInfo_getSdCardSize(android_avdInfo);
            if (!sdcard) {
                sdcard = avdInfo_getSdCardPath(android_avdInfo);
            }
            mData->avdDetails.append(StringFormat("SD Card: %s\n", sdcard));

            if (iniFile_hasKey(configIni, SNAPSHOT_PRESENT)) {
                mData->avdDetails.append(StringFormat(
                        "Snapshot: %s",
                        avdInfo_getSnapshotPresent(android_avdInfo) ? "yes"
                                                                    : "no"));
            }

            int totalEntry = iniFile_getPairCount(configIni);
            for (int idx = 0; idx < totalEntry; idx++) {
                char* key;
                char* value;
                if (!iniFile_getEntry(configIni, idx, &key, &value)) {
                    // check if the key is already displayed
                    auto end = avdDetailsNoDisplayKeys.end();
                    auto it = std::find_if(
                            avdDetailsNoDisplayKeys.begin(), end,
                            [key](const char* no_display_key) {
                                return strcmp(key, no_display_key) == 0;
                            });
                    if (it == end) {
                        mData->avdDetails.append(
                                StringFormat("%s: %s\n", key, value));
                    }
                }
            }
        }
    };
    bool mFirstTime = true;
    std::unique_ptr<BugreportData> mData;
    std::unique_ptr<AdbBugReportServices> mBugReportServices;
    ScreenCapturer* mScreenCapturer = nullptr;
};

static Bugreport sBugreport;

BugreportData* generateBugreport(BugreportCallback callback, void* context) {
    return sBugreport.generateBugreport(callback, context);
}

int saveBugreport(BugreportData* bugreport, int savingFlag) {
    bool res = sBugreport.saveBugreport(bugreport, savingFlag);
    return res ? 0 : -1;
}

int setSavePath(BugreportData* data, const char* savePath) {
    bool res = sBugreport.setSavePath(data, savePath);
    return res ? 0 : -1;
}

void setReproSteps(BugreportData* data, const char* reproSteps) {
    sBugreport.setReproSteps(data, reproSteps);
}

void bugReportInit(void* adbInterface, void* screencap) {
    AdbInterface* adb = static_cast<AdbInterface*>(adbInterface);
    ScreenCapturer* screenCapturer =
            screencap ? static_cast<ScreenCapturer*>(screencap) : nullptr;
    sBugreport.initialize(adb, screenCapturer);
}

// Return 1 if the bugreport has finished data collection otherwise 0
int bugreportIsReady(BugreportData* data) {
    return data->bugreportReady ? 1 : 0;
}
