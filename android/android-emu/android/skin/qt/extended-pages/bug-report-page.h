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

#include <qobjectdefs.h>                             // for Q_OBJECT, slots
#include <stddef.h>                                  // for size_t
#include <QString>                                   // for QString
#include <QWidget>                                   // for QWidget
#include <memory>                                    // for unique_ptr
#include <string>                                    // for string

#include "android/avd/BugreportInfo.h"               // for BugreportInfo
#include "android/base/StringView.h"                 // for StringView
#include "android/emulation/control/adb/AdbInterface.h"  // for AdbCommandPtr
#include "android/settings-agent.h"                  // for SettingsTheme

class QEvent;
class QMessageBox;
class QObject;
class QShowEvent;
class QWidget;
namespace Ui {
class BugreportPage;
}  // namespace Ui

class BugreportPage : public QWidget {
    Q_OBJECT

public:
    explicit BugreportPage(QWidget* parent = 0);
    ~BugreportPage();
    void setAdbInterface(android::emulation::AdbInterface* adb);
    void showEvent(QShowEvent* event) override;
    void updateTheme();

    struct SavingStates {
        std::string saveLocation;
        std::string adbBugreportFilePath;
        std::string screenshotFilePath;
        std::string bugreportFolderPath;
        bool adbBugreportSucceed;
        bool screenshotSucceed;
        bool bugreportSavedSucceed;
    };

private slots:
    void on_bug_saveButton_clicked();
    void on_bug_sendToGoogle_clicked();

private:
    void loadAdbBugreport();
    void loadAdbLogcat();
    void loadCircularSpinner(SettingsTheme theme);
    void loadScreenshotImage();
    bool eventFilter(QObject* object, QEvent* event) override;
    bool launchIssueTracker();
    void enableInput(bool enabled);
    bool saveBugReportTo(android::base::StringView location);
    bool saveToFile(android::base::StringView filePath,
                    const char* content,
                    size_t length);
    std::string generateUniqueBugreportName();
    android::emulation::AdbInterface* mAdb;
    QMessageBox* mDeviceDetailsDialog;
    bool mFirstShowEvent = true;
    std::unique_ptr<Ui::BugreportPage> mUi;
    SavingStates mSavingStates;
    android::avd::BugreportInfo mReportingFields;
    std::string mReproSteps;
    android::emulation::AdbCommandPtr mAdbBugreport;
    android::emulation::AdbCommandPtr mAdbLogcat;
};
