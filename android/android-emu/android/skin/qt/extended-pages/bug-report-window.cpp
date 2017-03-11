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

#include "android/globals.h"
#include "android/skin/qt/error-dialog.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/qt-settings.h"
#include "android/update-check/VersionExtractor.h"
#include "ui_bug-report-window.h"

#include <QSettings>

using android::base::StringView;
using android::base::System;
using android::emulation::AdbInterface;
using android::emulation::AdbBugReportServices;
using android::emulation::ScreenCapturer;

BugReportWindow::BugReportWindow(AdbInterface* adb,
                                 ScreenCapturer* screenCapturer,
                                 QWidget* parent)
    : QFrame(parent),
      mAdb(adb),
      mBugReportServices(adb),
      mScreenCapturer(screenCapturer),
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

    mBugReportSucceed = false;
    // Get the version of this code
    android::update_check::VersionExtractor vEx;
    android::base::Version curVersion = vEx.getCurrentVersion();
    auto verStr = curVersion.isValid() ? QString(curVersion.toString().c_str())
                                       : "Unknown";
    mUi->bug_emulatorVersionLabel->setText(verStr);

    char versionString[128];
    int apiLevel = avdInfo_getApiLevel(android_avdInfo);
    avdInfo_getFullApiName(apiLevel, versionString, 128);
    mUi->bug_androidVersionLabel->setText(QString(versionString));
}

void BugReportWindow::showEvent(QShowEvent* event) {
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
        prepareBugReportInBackground();
    } else {
        showErrorDialog(tr("The bugreport save location is not set.<br/>"
                           "Check the settings page and ensure the directory "
                           "exists and is writeable."),
                        tr("Bugreport"));
    }
}

void BugReportWindow::prepareBugReportInBackground() {
    mBugReportSucceed = false;
    mAdbBugreportFilePath = nullptr;
    mUi->bug_bugReportProgressBar->show();
    mBugReportServices.generateBugReport(
            mBugReportSaveLocation,
            [this](AdbBugReportServices::Result result, StringView filePath) {
                mUi->bug_bugReportProgressBar->hide();
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

    mUi->bug_screenshotProgressBar->show();
    mScreenCapturer->capture(
            mBugReportSaveLocation,
            [this](ScreenCapturer::Result result, StringView filePath) {
                BugReportWindow::loadScreenshotImageDone(result, filePath);
            });
}

void BugReportWindow::loadScreenshotImageDone(ScreenCapturer::Result result,
                                              StringView filePath) {
    mUi->bug_screenshotProgressBar->hide();
    if (result == ScreenCapturer::Result::kSuccess &&
        System::get()->pathIsFile(filePath) &&
        System::get()->pathCanRead(filePath)) {
        mScreenshotSucceed = true;
        mScreenshotFilePath = filePath;
        mUi->bug_screenshotImage->setScaledContents(true);
        QPixmap image(filePath.c_str());
        int height = mUi->bug_screenshotImage->height();
        int width = mUi->bug_screenshotImage->width();
        mUi->bug_screenshotImage->setPixmap(image.scaled(width, height));
    } else {
        // TODO(wdu) Better error handling for failed screen capture operation
        mUi->bug_screenshotImage->setText(
                tr("There was an error while capturing the screenshot."));
    }
}
