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

#include "ui_bug-report-window.h"
#include "android/globals.h"
#include "android/update-check/VersionExtractor.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/qt-settings.h"
#include "android/skin/qt/error-dialog.h"

#include <QSettings>

using namespace android::base;
using android::emulation::ScreenCapturer;
using android::emulation::AdbInterface;

BugReportWindow::BugReportWindow(QWidget* parent)
    : QFrame(parent),
      mUi(new Ui::BugReportWindow),
      mAdb(nullptr),
      mScreenCapturer(nullptr) {
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

    // Get the version of this code
    android::update_check::VersionExtractor vEx;
    android::base::Version curVersion = vEx.getCurrentVersion();
    auto verStr = curVersion.isValid() ? QString(curVersion.toString().c_str())
                                       : "Unknown";
    mUi->bug_emulatorVersionLabel->setText(verStr);

    int apiLevel = avdInfo_getApiLevel(android_avdInfo);
    char versionString[128];
    avdInfo_getFullApiName(apiLevel, versionString, 128);
    mUi->bug_androidVersionLabel->setText(QString(versionString));
}

void BugReportWindow::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    loadScreenshotImage();
}

void BugReportWindow::setAdbInterface(AdbInterface* adb) {
    mAdb = adb;
    if (mScreenCapturer) {
        free(mScreenCapturer);
    }
    mScreenCapturer = new ScreenCapturer(mAdb);
}

void BugReportWindow::loadScreenshotImage() {
    static const int MIN_SCREENSHOT_API = 14;
    if (avdInfo_getApiLevel(android_avdInfo) < MIN_SCREENSHOT_API) {
        showErrorDialog(tr("Screenshot is not supported below API 14."),
                        tr("Screenshot"));
        return;
    }

    QString savePath = getScreenshotSaveDirectory();
    if (savePath.isEmpty()) {
        showErrorDialog(tr("The screenshot save location is invalid.<br/>"
                           "Check the settings page and ensure the directory "
                           "exists and is writeable."),
                        tr("Screenshot"));
        return;
    }
    mUi->bug_screenshotImage->setText(tr("Loading..."));
    mScreenCapturer->capture(
            savePath.toStdString(),
            [this](ScreenCapturer::Result result, StringView filePath) {
                BugReportWindow::loadScreenshotImageDone(result, filePath);
            });
}

void BugReportWindow::loadScreenshotImageDone(ScreenCapturer::Result result,
                                              StringView filePath) {
    if (result == ScreenCapturer::Result::kSuccess &&
        System::get()->pathIsFile(filePath) &&
        System::get()->pathCanRead(filePath)) {
        mUi->bug_screenshotImage->setScaledContents(true);
        QPixmap image(filePath.c_str());
        int height = mUi->bug_screenshotImage->height();
        int width = mUi->bug_screenshotImage->width();
        mUi->bug_screenshotImage->setPixmap(image.scaled(width, height));
    }
}
