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
#include "android/base/Uri.h"
#include "android/base/files/PathUtils.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/misc/StringUtils.h"
#include "android/base/system/System.h"
#include "android/bugreport_data.h"
#include "android/emulation/ComponentVersion.h"
#include "android/emulation/ConfigDirs.h"
#include "android/emulation/CpuAccelerator.h"
#include "android/emulation/control/AdbBugReportServices.h"
#include "android/emulation/control/ScreenCapturer.h"
#include "android/emulator-window.h"
#include "android/globals.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/update-check/VersionExtractor.h"
#include "android/utils/path.h"

#include <algorithm>
#include <fstream>
#include <vector>

#include <QDesktopServices>
#include <QFileDialog>
#include <QObject>
#include <QString>
#include <QUrl>

using android::base::LazyInstance;
using android::base::PathUtils;
using android::base::StringFormat;
using android::base::StringView;
using android::base::System;
using android::base::Uri;
using android::base::Version;
using android::base::trim;
using android::emulation::AdbBugReportServices;
using android::emulation::AdbInterface;
using android::emulation::ScreenCapturer;

static const char FILE_BUG_URL[] =
        "https://issuetracker.google.com/issues/new"
        "?component=192727&description=%s&template=843117";

// In reference to
// https://developer.android.com/studio/report-bugs.html#emulator-bugs
static const char BUG_REPORT_TEMPLATE[] =
        R"(Please Read:
https://developer.android.com/studio/report-bugs.html#emulator-bugs

Android Studio Version:

Emulator Version (Emulator--> Extended Controls--> Emulator Version): %s

HAXM / KVM Version: %s

Android SDK Tools: %s

Host Operating System: %s

CPU Manufacturer: %s

Steps to Reproduce Bug:
%s

Expected Behavior:

Observed Behavior:)";

class Bugreport {
public:
    static Bugreport* getInstance();
    ~Bugreport() {
        if (System::get()->pathIsFile(mData.adbBugreportFilePath)) {
            System::get()->deleteFile(mData.adbBugreportFilePath);
        }
        if (System::get()->pathIsFile(mData.screenshotFilePath)) {
            System::get()->deleteFile(mData.screenshotFilePath);
        }
    };
    void initialize(AdbInterface* adb, bool hasWindow) {
        // In the case where no-window option is enabled, screen capturer
        // service is disabled by default.
        mBugReportServices.reset(new AdbBugReportServices(adb));
        if (hasWindow) {
            const int MIN_SCREENSHOT_API = 14;
            const int DEFAULT_API_LEVEL = 1000;
            int apiLevel = avdInfo_getApiLevel(android_avdInfo);
            if (apiLevel != DEFAULT_API_LEVEL &&
                apiLevel >= MIN_SCREENSHOT_API) {
                mScreenCapSupported = true;
                mScreenCapturer.reset(new ScreenCapturer(adb));
            }
        }
    };

    bool sendToGoogle() {
        std::string bugTemplate = StringFormat(
                BUG_REPORT_TEMPLATE, mData.emulatorVer, mData.hypervisorVer,
                mData.sdkToolsVer, mData.hostOsName, trim(mData.cpuModel),
                mData.reproSteps);
        std::string unEncodedUrl =
                Uri::FormatEncodeArguments(FILE_BUG_URL, bugTemplate);
        QUrl url(QString::fromStdString(unEncodedUrl));
        bool res = url.isValid() && QDesktopServices::openUrl(url);
        QFileDialog::getOpenFileName(
                Q_NULLPTR, QObject::tr("Report Saving Location"),
                QString::fromStdString(mData.bugreportFolderPath));
        return res;
    };

    void generateBugreport(BugreportCallback callback, void* context) {
        mData.adbBugreportSucceed = false;
        mData.adbLogactSucceed = false;
        mData.screenshotSucceed = false;
        mData.bugreportSavedSucceed = false;

        if (mFirstTime) {
            android::update_check::VersionExtractor vEx;
            Version curEmuVersion = vEx.getCurrentVersion();
            mData.emulatorVer = curEmuVersion.isValid()
                                        ? curEmuVersion.toString()
                                        : "Unknown";
            android::CpuAccelerator accel = android::GetCurrentCpuAccelerator();
            Version accelVersion = android::GetCurrentCpuAcceleratorVersion();
            if (accelVersion.isValid()) {
                mData.hypervisorVer = (StringFormat(
                        "%s %s", android::CpuAcceleratorToString(accel),
                        accelVersion.toString()));
            }

            char versionString[128];
            int apiLevel = avdInfo_getApiLevel(android_avdInfo);
            avdInfo_getFullApiName(apiLevel, versionString, 128);
            mData.androidVer = std::string(versionString);

            const char* deviceName = avdInfo_getName(android_avdInfo);
            mData.deviceName = std::string(deviceName);

            mData.hostOsName = System::get()->getOsName();

            mData.sdkToolsVer =
                    android::getCurrentSdkVersion(
                            android::ConfigDirs::getSdkRootDirectory(),
                            android::SdkComponentType::Tools)
                            .toString();

            mData.cpuModel = android::GetCpuInfo().second;

            loadAvdDetails();
            mFirstTime = false;
            if (callback) {
                callback(&mData, BugreportDataType::AvdDetails, context);
            }
        }

        if (!mBugReportServices->isLogcatInFlight()) {
            mBugReportServices->generateAdbLogcatInMemory(
                    [this, callback, context](
                            AdbBugReportServices::Result result,
                            StringView output) {
                        if (result == AdbBugReportServices::Result::Success) {
                            mData.adbLogactSucceed = true;
                            mData.adbLogcatOutput = output;
                        } else {
                            mData.adbLogactSucceed = false;
                        }
                        if (callback) {
                            callback(&mData, BugreportDataType::AdbLogcat,
                                     context);
                        }
                    });
        }

        if (mScreenCapSupported && !mScreenCapturer->isInFlight()) {
            mScreenCapturer->capture(
                    System::get()->getTempDir(),
                    [this, callback, context](ScreenCapturer::Result result,
                                              StringView filePath) {
                        if (System::get()->pathIsFile(
                                    mData.screenshotFilePath)) {
                            System::get()->deleteFile(mData.screenshotFilePath);
                            mData.screenshotFilePath.clear();
                        }
                        if (result == ScreenCapturer::Result::kSuccess &&
                            System::get()->pathIsFile(filePath) &&
                            System::get()->pathCanRead(filePath)) {
                            mData.screenshotSucceed = true;
                            mData.screenshotFilePath = filePath;
                        }
                        if (callback) {
                            callback(&mData, BugreportDataType::Screenshot,
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
                        if (System::get()->pathIsFile(
                                    mData.adbBugreportFilePath)) {
                            System::get()->deleteFile(
                                    mData.adbBugreportFilePath);
                            mData.adbBugreportFilePath.clear();
                        }
                        if (result == AdbBugReportServices::Result::Success &&
                            System::get()->pathIsFile(filePath) &&
                            System::get()->pathCanRead(filePath)) {
                            mData.adbBugreportSucceed = true;
                            mData.adbBugreportFilePath = filePath;
                        }
                        if (callback) {
                            callback(&mData, BugreportDataType::AdbBugreport,
                                     context);
                        }
                    });
        }
    };

    bool saveBugreport(bool includeScreenshot, bool includeAdbBugreport) {
        if (mData.bugreportSavedSucceed)
            return true;

        if (mData.saveLocation.empty()) {
            mData.saveLocation = getScreenshotSaveDirectory().toStdString();
        }

        mData.bugreportFolderPath = PathUtils::join(
                mData.saveLocation,
                AdbBugReportServices::generateUniqueBugreportName());

        if (path_mkdir_if_needed(mData.bugreportFolderPath.c_str(), 0755)) {
            return false;
        }

        if (includeAdbBugreport && !mData.adbBugreportFilePath.empty()) {
            std::string bugreportBaseName;
            if (PathUtils::extension(mData.adbBugreportFilePath) == ".zip") {
                bugreportBaseName = "bugreport.zip";
            } else {
                bugreportBaseName = "bugreport.txt";
            }
            path_copy_file(PathUtils::join(mData.bugreportFolderPath,
                                           bugreportBaseName)
                                   .c_str(),
                           mData.adbBugreportFilePath.c_str());
        }

        if (includeScreenshot && !mData.screenshotFilePath.empty()) {
            std::string screenshotBaseName = "screenshot.png";
            path_copy_file(PathUtils::join(mData.bugreportFolderPath,
                                           screenshotBaseName)
                                   .c_str(),
                           mData.screenshotFilePath.c_str());
        }

        if (!mData.avdDetails.empty()) {
            auto avdDetailsFilePath = PathUtils::join(mData.bugreportFolderPath,
                                                      "avd_details.txt");
            std::ofstream outFile(avdDetailsFilePath.c_str(),
                                  std::ios_base::out | std::ios_base::trunc);
            outFile << mData.avdDetails << std::endl;
        }

        if (!mData.reproSteps.empty()) {
            auto reproStepsFilePath = PathUtils::join(mData.bugreportFolderPath,
                                                      "repro_steps.txt");
            std::ofstream outFile(reproStepsFilePath.c_str(),
                                  std::ios_base::out | std::ios_base::trunc);
            outFile << mData.reproSteps << std::endl;
        }
        mData.bugreportSavedSucceed = true;
        return true;
    };

    void setReproSteps(const char* reproSteps) {
        mData.reproSteps = reproSteps;
    };

    bool setSavePath(const char* savePath) {
        if (System::get()->pathIsDir(savePath) &&
            System::get()->pathCanWrite(savePath)) {
            mData.saveLocation = savePath;
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
            mData.avdDetails.append(
                    StringFormat("Name: %s\n", mData.deviceName));
            mData.avdDetails.append(
                    StringFormat("CPU/ABI: %s\n",
                                 avdInfo_getTargetCpuArch(android_avdInfo)));
            mData.avdDetails.append(StringFormat(
                    "Path: %s\n", avdInfo_getContentPath(android_avdInfo)));
            mData.avdDetails.append(
                    StringFormat("Target: %s (API level %d)\n",
                                 avdInfo_getTag(android_avdInfo),
                                 avdInfo_getApiLevel(android_avdInfo)));
            char* skinName;
            char* skinDir;
            avdInfo_getSkinInfo(android_avdInfo, &skinName, &skinDir);
            if (skinName)
                mData.avdDetails.append(StringFormat("Skin: %s\n", skinName));

            const char* sdcard = avdInfo_getSdCardSize(android_avdInfo);
            if (!sdcard) {
                sdcard = avdInfo_getSdCardPath(android_avdInfo);
            }
            mData.avdDetails.append(StringFormat("SD Card: %s\n", sdcard));

            if (iniFile_hasKey(configIni, SNAPSHOT_PRESENT)) {
                mData.avdDetails.append(StringFormat(
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
                        mData.avdDetails.append(
                                StringFormat("%s: %s\n", key, value));
                    }
                }
            }
        }
    };
    Bugreport(){};
    bool mFirstTime = true;
    bool mScreenCapSupported = false;
    BugreportData mData;
    std::unique_ptr<AdbBugReportServices> mBugReportServices;
    std::unique_ptr<ScreenCapturer> mScreenCapturer;
};

static std::unique_ptr<Bugreport> sBugreport;

Bugreport* Bugreport::getInstance() {
    if (nullptr == sBugreport.get()) {
        sBugreport.reset(new Bugreport());
    }
    return sBugreport.get();
};

void generateBugreport(BugreportCallback callback, void* context) {
    Bugreport::getInstance()->generateBugreport(callback, context);
}

int saveBugreport(int includeScreenshot, int includeAdbBugreport) {
    bool res = Bugreport::getInstance()->saveBugreport(
            (bool)includeScreenshot, (bool)includeAdbBugreport);
    return res ? 0 : -1;
}

int sendToGoogle() {
    bool res = Bugreport::getInstance()->sendToGoogle();
    return res ? 0 : -1;
}

void setReproSteps(const char* reproSteps) {
    Bugreport::getInstance()->setReproSteps(reproSteps);
}

int setSavePath(const char* savePath) {
    bool res = Bugreport::getInstance()->setSavePath(savePath);
    return res ? 0 : -1;
}

void bugReportInit(void* adbInterface, int hasWindow) {
    AdbInterface* adb = static_cast<AdbInterface*>(adbInterface);
    Bugreport::getInstance()->initialize(adb, hasWindow == 0 ? false : true);
}
