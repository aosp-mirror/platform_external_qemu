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
#include "android/skin/qt/emulator-qt-window.h"

#include "ui_bug-report-window.h"

#include <QFrame>
#include <QMessageBox>
#include <QThread>

#include <memory>
#include <string>

class BugReportWindow : public QFrame {
    Q_OBJECT

public:
    explicit BugReportWindow(EmulatorQtWindow* eW, QWidget* parent = 0);
    void showEvent(QShowEvent* event);

private slots:
    void on_bug_saveButton_clicked();
    void on_bug_fileBugButton_clicked();
    void saveBugreportFolderFinished(bool reasult,
                                     QString folderPath,
                                     bool willOpenIssueTracker);
    void saveBugreportFolderStarted();

private:
    void loadAdbBugreport();
    void loadAdbLogcat();
    void loadAvdDetails();
    void loadScreenshotImage();
    bool eventFilter(QObject* object, QEvent* event) override;
    void saveBugReportFolder(const QObject* handler,
                             const char* started_slot,
                             const char* finished_slot,
                             bool willOpenIssueTracker);

    EmulatorQtWindow* mEmulatorWindow;
    android::emulation::AdbInterface* mAdb;
    android::emulation::AdbBugReportServices mBugReportServices;
    android::emulation::ScreenCapturer* mScreenCapturer;
    QMessageBox* mDeviceDetailsDialog;
    std::unique_ptr<Ui::BugReportWindow> mUi;
    bool mAdbBugreportSucceed;
    std::string mAdbBugreportFilePath;
    bool mScreenshotSucceed;
    std::string mScreenshotFilePath;
    std::string mSaveLocation;
    std::string mBugreportFolderPath;
    bool mBugreportSavedSucceed;
    std::string mEmulatorVer;
    std::string mAndroidVer;
    std::string mHostOsName;
    std::string mDeviceName;
    std::string mSdkToolsVer;
    std::string mReproSteps;
    bool mFirstShowEvent = true;
    std::string mAvdDetails;
};

class BugReportFolderSavingThread : public QThread {
    Q_OBJECT
public:
    void saveBugReportFromFiles(std::string savingLocation,
                                std::string adbBugreportFilePath,
                                std::string screenshotFilePath,
                                std::string avdDetails,
                                bool willOpenIssueTracker);

    static BugReportFolderSavingThread* newInstance(const QObject* handler,
                                                    const char* started_slot,
                                                    const char* finished_slot);
signals:
    void finished(bool savingResult, QString folderPath, bool openIssueTracker);

protected:
    void run() override;

private:
    BugReportFolderSavingThread() = default;
    std::string mSavingLocation;
    std::string mAdbBugreportFilePath;
    std::string mScreenshotFilePath;
    std::string mAvdDetails;
    bool mOpenIssueTracker;
};

class IssueTrackerThread : public QThread {
    Q_OBJECT
public:
    void openIssueTracker(std::string reproSteps,
                          std::string emulatorVer,
                          std::string sdkToolsVer,
                          std::string hostOsName);

    static IssueTrackerThread* newInstance();

protected:
    void run() override;

private:
    IssueTrackerThread() = default;
    std::string mReproSteps;
    std::string mEmulatorVer;
    std::string mSdkToolsVer;
    std::string mHostOsName;
};
