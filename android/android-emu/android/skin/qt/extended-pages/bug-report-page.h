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

#include "android/base/StringView.h"
#include "android/crashreport/CrashService.h"
#include "android/emulation/control/AdbInterface.h"
#include "android/skin/qt/extended-window.h"

#include "ui_bug-report-page.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QWidget>

#include <memory>
#include <string>

class UploadDialog;

class BugreportPage : public QWidget {
    Q_OBJECT

public:
    explicit BugreportPage(QWidget* parent = 0);
    ~BugreportPage();
    void setAdbInterface(android::emulation::AdbInterface* adb);
    void setExtendedWindow(ExtendedWindow* extendedWindow);
    void showEvent(QShowEvent* event);
    void updateTheme();
    struct ReportingFields {
        std::string androidVer;
        std::string avdDetails;
        std::string cpuModel;
        std::string deviceName;
        std::string emulatorVer;
        std::string hypervisorVer;
        std::string hostOsName;
        std::string reproSteps;
        std::string sdkToolsVer;
    };

    struct SavingStates {
        std::string crashReporterLocation;
        std::string saveLocation;
        std::string adbBugreportFilePath;
        std::string screenshotFilePath;
        std::string bugreportFolderPath;
        std::string adbLogcatOutput;
        bool adbBugreportSucceed;
        bool screenshotSucceed;
    };

private slots:
    void on_bug_saveButton_clicked();
    void on_bug_sendToGoogle_clicked();

private:
    void loadAdbBugreport();
    void loadAdbLogcat();
    void loadAvdDetails();
    void loadCircularSpinner(SettingsTheme theme);
    void loadScreenshotImage();
    bool eventFilter(QObject* object, QEvent* event) override;
    bool launchIssueTracker();
    void enableInput(bool enabled);
    bool saveBugReportTo(const std::string& location);
    bool compressToFile(android::base::StringView filePath,
                        const uint8_t* src,
                        size_t srcLength);
    bool saveToFile(android::base::StringView filePath,
                    const char* content,
                    size_t length);
    std::string generateUniqueBugreportName();

    ExtendedWindow* mExtendedWindow;
    android::emulation::AdbInterface* mAdb;
    QMessageBox* mDeviceDetailsDialog;
    UploadDialog* mUploadDialog = nullptr;
    bool mFirstShowEvent = true;
    std::unique_ptr<Ui::BugreportPage> mUi;
    ReportingFields mReportingFields;
    SavingStates mSavingStates;
    android::emulation::AdbCommandPtr mAdbBugreport;
    android::emulation::AdbCommandPtr mAdbLogcat;
};

class UploadDialog : public QDialog {
    Q_OBJECT
public:
    UploadDialog(QWidget* parent, android::crashreport::CrashService* service);
    const char* SEND_TO_GOOGLE =
            "Do you want to send the bug report to Google?";
    const char* SENDING_REPORT =
            "Sending bug report... This might take a minute.";
public slots:
    void sendBugReport();

private:
    void showEvent(QShowEvent* e) override;
    void collectSysInfo();
    QProgressBar* mProgressBar;
    QLabel* mDialogLabel;
    QDialogButtonBox* mButtonBox;
    std::unique_ptr<android::crashreport::CrashService> mCrashService;
};
