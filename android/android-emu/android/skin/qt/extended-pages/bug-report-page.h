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

#include "android/crashreport/CrashService.h"
#include "android/emulation/control/AdbBugReportServices.h"
#include "android/skin/qt/emulator-qt-window.h"

#include "ui_bug-report-page.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QWidget>

#include <memory>
#include <string>

class BugReportFolderSavingTask;
class UploadDialog;

class BugreportPage : public QWidget {
    Q_OBJECT

public:
    explicit BugreportPage(QWidget* parent = 0);
    ~BugreportPage();
    void initialize(EmulatorQtWindow* eW);
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
        bool adbBugreportSucceed;
        bool screenshotSucceed;
        bool bugreportSavedSucceed;
    };

private slots:
    void on_bug_saveButton_clicked();
    void on_bug_sendToGoogle_clicked();
    void saveBugreportFolderFinished(bool success);
    void saveBugreportFolderStarted();
    void saveBugreportToCrashDirFinished(bool success);
    void issueTrackerTaskFinished(bool success, QString error);

private:
    void loadAdbBugreport();
    void loadAdbLogcat();
    void loadAvdDetails();
    void loadCircularSpinner(SettingsTheme theme);
    void loadScreenshotImage();
    bool eventFilter(QObject* object, QEvent* event) override;
    BugReportFolderSavingTask* createBugreportFolderSavingTask(std::string s);
    void launchIssueTracker();

    EmulatorQtWindow* mEmulatorWindow;
    std::unique_ptr<android::emulation::AdbBugReportServices> mBugReportServices;
    QMessageBox* mDeviceDetailsDialog;
    UploadDialog* mUploadDialog;

    bool mFirstShowEvent = true;
    std::unique_ptr<Ui::BugreportPage> mUi;
    ReportingFields mReportingFields;
    SavingStates mSavingStates;
};

class BugReportFolderSavingTask : public QObject {
    Q_OBJECT
public:
    BugReportFolderSavingTask(std::string savingPath,
                              std::string adbBugreportFilePath,
                              std::string screenshotFilePath,
                              std::string avdDetails,
                              std::string reproSteps);
public slots:
    bool run();

signals:
    void started();
    void finished(bool success);

private:
    std::string mSavingPath;
    std::string mAdbBugreportFilePath;
    std::string mScreenshotFilePath;
    std::string mAvdDetails;
    std::string mReproSteps;
};

class IssueTrackerTask : public QObject {
    Q_OBJECT
public:
    IssueTrackerTask(BugreportPage::ReportingFields reportingField);

public slots:
    void run();
signals:
    void finished(bool success, QString error);

private:
    BugreportPage::ReportingFields mReportingFields;
};

class UploadDialog : public QDialog {
    Q_OBJECT
public:
    UploadDialog(QWidget* parent, android::crashreport::CrashService* service);
    const char* SEND_TO_GOOGLE =
            "Do you want to send the bug report to Google?";
    const char* COLLECT_INFO = "Collecting info ...";
    const char* SENDING_REPORT = "Sending bug report...";
public slots:
    void sendBugReport();

private:
    void collectSysInfo();
    QProgressBar* mProgressBar;
    QLabel* mDialogLabel;
    QDialogButtonBox* mButtonBox;
    std::unique_ptr<android::crashreport::CrashService> mCrashService;
};
