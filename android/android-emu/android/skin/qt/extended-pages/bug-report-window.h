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

#pragma once

#include "android/emulation/control/AdbBugReportServices.h"
#include "android/emulation/control/ScreenCapturer.h"
#include "ui_bug-report-window.h"

#include <QFrame>
#include <atomic>
#include <memory>

class BugReportWindow : public QFrame {
    Q_OBJECT

public:
    explicit BugReportWindow(android::emulation::AdbInterface* adb,
                             android::emulation::ScreenCapturer* screenCapturer,
                             QWidget* parent = 0);
    void showEvent(QShowEvent* event);

private:
    void prepareBugReportInBackground();
    void loadAdbLogcat();
    void loadScreenshotImage();
    void loadScreenshotImageDone(
            android::emulation::ScreenCapturer::Result result,
            android::base::StringView filePath);

    android::emulation::AdbInterface* mAdb;
    android::emulation::AdbBugReportServices mBugReportServices;
    android::emulation::ScreenCapturer* mScreenCapturer;
    std::unique_ptr<Ui::BugReportWindow> mUi;
    std::atomic_bool mBugReportSucceed;
    std::atomic_bool mScreenshotSucceed;
    std::string mBugReportSaveLocation;
    android::base::StringView mAdbBugreportFilePath;
    android::base::StringView mScreenshotFilePath;
};
