// Copyright (C) 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/extended-pages/bug-report-window.h"

#include "android/avd/keys.h"
#include "android/base/StringFormat.h"
#include "android/globals.h"
#include "android/skin/qt/error-dialog.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/qt-settings.h"
#include "android/skin/qt/stylesheet.h"
#include "android/update-check/VersionExtractor.h"

#include "ui_bug-report-window.h"

#include <QMovie>
#include <QSettings>

#include <algorithm>
#include <vector>

using android::base::StringFormat;
using android::base::StringView;
using android::base::System;
using android::emulation::AdbInterface;
using android::emulation::AdbBugReportServices;
using android::emulation::ScreenCapturer;

BugReportWindow::BugReportWindow(EmulatorQtWindow* eW, QWidget* parent)
    : QFrame(parent),
      mEmulatorWindow(eW),
      mAdb(eW->getAdbInterface()),
      mBugReportServices(mAdb),
      mScreenCapturer(eW->getScreenCapturer()),
      mUi(new Ui::BugReportWindow) {
// "Tool" type windows live in another layer on top of everything in OSX, which
// is undesirable because it means the extended window must be on top of the
// emulator window. However, on Windows and Linux, "Tool" type windows are the
// only way to make a window that does not have its own taskbar item.
#ifdef __APPLE__
    Qt::WindowFlags flag = Qt::Dialog;
#else
    Qt::WindowFlags flag = Qt::Tool;
#endif

    setWindowFlags(flag | Qt::WindowMinimizeButtonHint |
                   Qt::WindowCloseButtonHint);

    QSettings settings;
    bool onTop = settings.value(Ui::Settings::ALWAYS_ON_TOP, false).toBool();
    setFrameOnTop(this, onTop);

    mUi->setupUi(this);

    mUi->bug_deviceLabel->installEventFilter(this);

    android::update_check::VersionExtractor vEx;
    android::base::Version curVersion = vEx.getCurrentVersion();
    auto verStr = curVersion.isValid() ? QString(curVersion.toString().c_str())
                                       : "Unknown";
    mUi->bug_emulatorVersionLabel->setText(verStr);
    mEmulatorVer = curVersion.isValid() ? curVersion.toString() : "";

    char versionString[128];
    int apiLevel = avdInfo_getApiLevel(android_avdInfo);
    avdInfo_getFullApiName(apiLevel, versionString, 128);
    mUi->bug_androidVersionLabel->setText(QString(versionString));
    mAndroidVer = std::string(versionString);

    const char* deviceName = avdInfo_getName(android_avdInfo);
    mDeviceName = std::string(deviceName);
    mUi->bug_deviceLabel->setText(QString(deviceName));

    mHostOsName = android::base::toString(System::get()->getOsType());
    mUi->bug_hostMachineLabel->setText(QString(mHostOsName.c_str()));

    SettingsTheme theme = getSelectedTheme();
    QMovie* movie = new QMovie(this);
    movie->setFileName(":/" + Ui::stylesheetValues(theme)[Ui::THEME_PATH_VAR] +
                       "/circular_spinner");
    if (movie->isValid()) {
        movie->start();
        mUi->bug_circularSpinner->setMovie(movie);
        mUi->bug_circularSpinner->hide();
        mUi->bug_collectingLabel->hide();
        mUi->bug_circularSpinner2->setMovie(movie);
        mUi->bug_circularSpinner2->show();
    }

    loadAvdDetails();
    mDeviceDetailsDialog = new QMessageBox(
            QMessageBox::NoIcon,
            QString(StringFormat("Details for %s", deviceName).c_str()), "",
            QMessageBox::Close, this);
    mDeviceDetailsDialog->setModal(false);
    mDeviceDetailsDialog->setInformativeText(QString(mAvdDetails.c_str()));
}

void BugReportWindow::showEvent(QShowEvent* event) {
    // Align the left side of bugreport window with the extended window.
    if (mFirstShowEvent && !event->spontaneous()) {
        ToolWindow* tW = mEmulatorWindow->toolWindow();
        move(tW->geometry().right() + ToolWindow::toolGap,
             tW->geometry().top());
        mFirstShowEvent = false;
    }
    QWidget::showEvent(event);
    // clean up the content loaded from last time.
    mUi->bug_bugReportTextEdit->clear();
    mUi->bug_reproStepsTextEdit->clear();
    mUi->bug_screenshotImage->clear();
    // check if save location is set
    mBugReportSaveLocation = getScreenshotSaveDirectory().toStdString();
    if (!mBugReportSaveLocation.empty() &&
        System::get()->pathIsDir(mBugReportSaveLocation) &&
        System::get()->pathCanWrite(mBugReportSaveLocation)) {
        loadAdbLogcat();
        loadScreenshotImage();
        loadAdbBugreport();
    } else {
        showErrorDialog(tr("The bugreport save location is not set.<br/>"
                           "Check the settings page and ensure the directory "
                           "exists and is writeable."),
                        tr("Bugreport"));
    }
}

void BugReportWindow::loadAdbBugreport() {
    mBugReportSucceed = false;
    SettingsTheme theme = getSelectedTheme();
    setButtonEnabled(mUi->bug_fileBugButton, theme, false);
    setButtonEnabled(mUi->bug_saveButton, theme, false);
    mUi->bug_circularSpinner->show();
    mUi->bug_collectingLabel->show();

    mBugReportServices.generateBugReport(
            mBugReportSaveLocation,
            [this, theme](AdbBugReportServices::Result result,
                          StringView filePath) {
                setButtonEnabled(mUi->bug_fileBugButton, theme, true);
                setButtonEnabled(mUi->bug_saveButton, theme, true);
                mUi->bug_circularSpinner->hide();
                mUi->bug_collectingLabel->hide();

                if (result == AdbBugReportServices::Result::Success &&
                    System::get()->pathIsFile(filePath) &&
                    System::get()->pathCanRead(filePath)) {
                    mBugReportSucceed = true;
                    mAdbBugreportFilePath = filePath;
                } else {
                    // TODO(wdu) Better error handling for failed adb bugreport
                    // generation
                    showErrorDialog(tr("There was an error while generating "
                                       "adb bugreport"),
                                    tr("Bugreport"));
                }
            });
}

void BugReportWindow::loadAdbLogcat() {
    mBugReportServices.generateAdbLogcatInMemory(
            [this](AdbBugReportServices::Result result, StringView output) {
                if (result == AdbBugReportServices::Result::Success) {
                    mUi->bug_bugReportTextEdit->setPlainText(
                            tr(output.c_str()));
                } else {
                    // TODO(wdu) Better error handling for failed adb logcat
                    // generation
                    mUi->bug_bugReportTextEdit->setPlainText(
                            tr("There was an error while loading adb logact"));
                }
            });
}

void BugReportWindow::loadAvdDetails() {
    static std::vector<const char*> avdDetailsNoDisplayKeys{
            ABI_TYPE,    CPU_ARCH,    SKIN_NAME, SKIN_PATH,
            SDCARD_SIZE, SDCARD_PATH, IMAGES_2};

    CIniFile* configIni = avdInfo_getConfigIni(android_avdInfo);

    if (configIni) {
        mAvdDetails.append(StringFormat("Name: %s\n", mDeviceName));
        mAvdDetails.append(StringFormat(
                "CPU/ABI: %s\n", avdInfo_getTargetCpuArch(android_avdInfo)));
        mAvdDetails.append(StringFormat(
                "Path: %s\n", avdInfo_getContentPath(android_avdInfo)));
        mAvdDetails.append(StringFormat("Target: %s (API level %d)\n",
                                        avdInfo_getTag(android_avdInfo),
                                        avdInfo_getApiLevel(android_avdInfo)));
        char* skinName;
        char* skinDir;
        avdInfo_getSkinInfo(android_avdInfo, &skinName, &skinDir);
        if (skinName)
            mAvdDetails.append(StringFormat("Skin: %s\n", skinName));

        const char* sdcard = avdInfo_getSdCardSize(android_avdInfo);
        if (!sdcard) {
            sdcard = avdInfo_getSdCardPath(android_avdInfo);
        }
        mAvdDetails.append(StringFormat("SD Card: %s\n", sdcard));

        if (iniFile_hasKey(configIni, SNAPSHOT_PRESENT)) {
            mAvdDetails.append(StringFormat(
                    "Snapshot: %s", avdInfo_getSnapshotPresent(android_avdInfo)
                                            ? "yes"
                                            : "no"));
        }

        int totalEntry = iniFile_getPairCount(configIni);
        for (int idx = 0; idx < totalEntry; idx++) {
            char* key;
            char* value;

            if (!iniFile_getEntry(configIni, idx, &key, &value)) {
                // check if the key is already displayed
                auto end = avdDetailsNoDisplayKeys.end();
                auto it = std::find_if(avdDetailsNoDisplayKeys.begin(), end,
                                       [key](const char* no_display_key) {
                                           return strcmp(key, no_display_key) ==
                                                  0;
                                       });
                if (it == end) {
                    mAvdDetails.append(StringFormat("%s: %s\n", key, value));
                }
            }
        }
    }
}

void BugReportWindow::loadScreenshotImage() {
    static const int MIN_SCREENSHOT_API = 14;
    static const int DEFAULT_API_LEVEL = 1000;
    mScreenshotSucceed = false;
    int apiLevel = avdInfo_getApiLevel(android_avdInfo);
    if (apiLevel == DEFAULT_API_LEVEL || apiLevel < MIN_SCREENSHOT_API) {
        mUi->bug_screenshotImage->setText(
                tr("Screenshot is not supported below API 14."));
        return;
    }
    mUi->stackedWidget->setCurrentIndex(1);
    mScreenCapturer->capture(
            mBugReportSaveLocation,
            [this](ScreenCapturer::Result result, StringView filePath) {
                BugReportWindow::loadScreenshotImageDone(result, filePath);
            });
}

void BugReportWindow::loadScreenshotImageDone(ScreenCapturer::Result result,
                                              StringView filePath) {
    mUi->stackedWidget->setCurrentIndex(0);
    if (result == ScreenCapturer::Result::kSuccess &&
        System::get()->pathIsFile(filePath) &&
        System::get()->pathCanRead(filePath)) {
        mScreenshotSucceed = true;
        mScreenshotFilePath = filePath;
        QPixmap image(filePath.c_str());

        int height = mUi->bug_screenshotImage->height();
        int width = mUi->bug_screenshotImage->width();
        mUi->bug_screenshotImage->setPixmap(
                image.scaled(width, height, Qt::KeepAspectRatio));
    } else {
        // TODO(wdu) Better error handling for failed screen capture operation
        mUi->bug_screenshotImage->setText(
                tr("There was an error while capturing the screenshot."));
    }
}

bool BugReportWindow::eventFilter(QObject* object, QEvent* event) {
    if (event->type() != QEvent::MouseButtonPress) {
        return false;
    }
    mDeviceDetailsDialog->show();
    return true;
}
