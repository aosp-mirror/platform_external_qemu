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

#include <stddef.h>
#include <QString>
#include <QWidget>
#include <memory>
#include <string>
#include <string_view>

#include "aemu/base/async/RecurrentTask.h"
#include "android/avd/BugreportInfo.h"
#include "android/emulation/control/adb/AdbInterface.h"
#include "android/emulation/control/utils/EmulatorControlClient.h"
#include "android/skin/qt/themed-widget.h"
#include "host-common/qt_ui_defs.h"

namespace android {
namespace emulation {
namespace control {
class Image;
}  // namespace control
}  // namespace emulation

namespace metrics {
class UiEventTracker;
}  // namespace metrics
}  // namespace android

using android::emulation::control::Image;
using android::emulation::control::EmulatorControlClient;
using android::metrics::UiEventTracker;

namespace Ui {
class BugreportPageGrpc;
}  // namespace Ui

constexpr int kDefaultUnknownAPILevel = 1000;

class DeviceDetailPage;

class BugreportPageGrpc : public ThemedWidget {
    Q_OBJECT

public:
    explicit BugreportPageGrpc(QWidget* parent = 0);
    ~BugreportPageGrpc();
    void setAdbInterface(android::emulation::AdbInterface* adb);
    void showEvent(QShowEvent* event) override;
    void updateTheme() override;

    struct SavingStates {
        std::string saveLocation;
        std::string adbBugreportFilePath;
        std::string screenshotFilePath;
        std::string bugreportFolderPath;
        bool adbBugreportSucceed;
        bool screenshotSucceed;
        bool bugreportSavedSucceed;
    };

signals:
    void imageLoaded();

private slots:
    void on_bug_saveButton_clicked();
    void on_bug_sendToGoogle_clicked();
    void on_bug_bugReportCheckBox_clicked();
    void on_imageLoaded();

private:
    void getBootStatus();
    void refreshContents();
    void loadAdbBugreport();
    void loadAdbLogcat();
    void loadCircularSpinner(SettingsTheme theme);
    void loadScreenshotImage();
    bool eventFilter(QObject* object, QEvent* event) override;
    bool launchIssueTracker();
    void enableInput(bool enabled);
    void showSpinningWheelAnimation(bool enabled);
    bool saveBugReportTo(std::string_view location);
    bool saveToFile(std::string_view filePath,
                    const char* content,
                    size_t length);
    std::string generateUniqueBugreportName();
    android::emulation::AdbInterface* mAdb = nullptr;
    std::unique_ptr<DeviceDetailPage> mDeviceDetail;
    std::unique_ptr<Ui::BugreportPageGrpc> mUi;
    std::shared_ptr<UiEventTracker> mBugTracker;
    SavingStates mSavingStates;
    android::avd::BugreportInfo mReportingFields;
    std::string mReproSteps;
    android::emulation::AdbCommandPtr mAdbBugreport;
    android::emulation::AdbCommandPtr mAdbLogcat;
    EmulatorControlClient mEmulatorControl;
    android::base::RecurrentTask mTask;
    bool mBooted{false};
    int mApiLevel{kDefaultUnknownAPILevel};
    std::string mDeviceName{"UNKNOWN_DEVICE"};
};
