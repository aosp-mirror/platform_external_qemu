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

#include "android/skin/qt/extended-pages/bug-report-page.h"

#include "android/avd/keys.h"
#include "android/base/StringFormat.h"
#include "android/base/Uri.h"
#include "android/base/files/PathUtils.h"
#include "android/base/misc/StringUtils.h"
#include "android/emulation/ComponentVersion.h"
#include "android/emulation/ConfigDirs.h"
#include "android/emulation/CpuAccelerator.h"
#include "android/globals.h"
#include "android/skin/qt/error-dialog.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/qt-settings.h"
#include "android/skin/qt/stylesheet.h"
#include "android/update-check/VersionExtractor.h"
#include "android/utils/file_io.h"
#include "android/utils/path.h"
#include "ui_bug-report-page.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QFontMetrics>
#include <QMovie>

#include <algorithm>
#include <fstream>
#include <vector>
using android::base::PathUtils;
using android::base::StringFormat;
using android::base::StringView;
using android::base::System;
using android::base::trim;
using android::base::Uri;
using android::base::Version;
using android::emulation::AdbInterface;
using android::emulation::AdbBugReportServices;
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

BugreportPage::BugreportPage(QWidget* parent)
    : QWidget(parent), mUi(new Ui::BugreportPage) {
    mUi->setupUi(this);
    mUi->bug_deviceLabel->installEventFilter(this);

    // Set emulator version and affix it with CPU accelerator version if
    // applicable
    android::update_check::VersionExtractor vEx;
    Version curEmuVersion = vEx.getCurrentVersion();
    mReportingFields.emulatorVer =
            curEmuVersion.isValid() ? curEmuVersion.toString() : "Unknown";
    android::CpuAccelerator accel = android::GetCurrentCpuAccelerator();
    Version accelVersion = android::GetCurrentCpuAcceleratorVersion();
    if (accelVersion.isValid()) {
        mReportingFields.hypervisorVer =
                (StringFormat("%s %s", android::CpuAcceleratorToString(accel),
                              accelVersion.toString()));
    }
    mUi->bug_emulatorVersionLabel->setText(QString::fromStdString(
            StringFormat("%s (%s)", mReportingFields.emulatorVer,
                         mReportingFields.hypervisorVer)));

    // Set android guest system version
    char versionString[128];
    int apiLevel = avdInfo_getApiLevel(android_avdInfo);
    avdInfo_getFullApiName(apiLevel, versionString, 128);
    mUi->bug_androidVersionLabel->setText(QString(versionString));
    mReportingFields.androidVer = std::string(versionString);

    // Set AVD name
    const char* deviceName = avdInfo_getName(android_avdInfo);
    mReportingFields.deviceName = std::string(deviceName);
    QFontMetrics metrics(mUi->bug_deviceLabel->font());
    QString elidedText = metrics.elidedText(
            QString::fromStdString(mReportingFields.deviceName), Qt::ElideRight,
            mUi->bug_deviceLabel->width());
    mUi->bug_deviceLabel->setText(elidedText);

    // Set OS Description
    mReportingFields.hostOsName = System::get()->getOsName();
    mUi->bug_hostMachineLabel->setText(
            QString::fromStdString(mReportingFields.hostOsName));

    // Set SDK tools version
    mReportingFields.sdkToolsVer =
            android::getCurrentSdkVersion(
                    android::ConfigDirs::getSdkRootDirectory(),
                    android::SdkComponentType::Tools)
                    .toString();

    // Set CPU model
    mReportingFields.cpuModel = android::GetCpuInfo().second;

    SettingsTheme theme = getSelectedTheme();
    QMovie* movie = new QMovie(this);
    movie->setFileName(":/" + Ui::stylesheetValues(theme)[Ui::THEME_PATH_VAR] +
                       "/circular_spinner");
    if (movie->isValid()) {
        movie->start();
        mUi->bug_circularSpinner->setMovie(movie);
        mUi->bug_circularSpinner2->setMovie(movie);
    }

    loadAvdDetails();
    mDeviceDetailsDialog = new QMessageBox(
            QMessageBox::NoIcon,
            QString::fromStdString(StringFormat("Details for %s", deviceName)),
            "", QMessageBox::Close, this);
    mDeviceDetailsDialog->setModal(false);
    mDeviceDetailsDialog->setInformativeText(
            QString::fromStdString(mReportingFields.avdDetails));
    mUi->bug_sendToGoogle->setIcon(getIconForCurrentTheme("open_in_browser"));
}

BugreportPage::~BugreportPage() {
    if (System::get()->pathIsFile(mSavingStates.adbBugreportFilePath)) {
        System::get()->deleteFile(mSavingStates.adbBugreportFilePath);
    }
    if (System::get()->pathIsFile(mSavingStates.screenshotFilePath)) {
        System::get()->deleteFile(mSavingStates.screenshotFilePath);
    }
}

void BugreportPage::initialize(EmulatorQtWindow* eW) {
    mEmulatorWindow = eW;
    mBugReportServices.reset(new AdbBugReportServices(eW->getAdbInterface()));
    mScreenCapturer = eW->getScreenCapturer();
}

void BugreportPage::updateTheme() {
    SettingsTheme theme = getSelectedTheme();
    QMovie* movie = new QMovie(this);
    movie->setFileName(":/" + Ui::stylesheetValues(theme)[Ui::THEME_PATH_VAR] +
                       "/circular_spinner");
    if (movie->isValid()) {
        movie->start();
        mUi->bug_circularSpinner->setMovie(movie);
        mUi->bug_circularSpinner2->setMovie(movie);
    }

    mUi->bug_bugReportTextEdit->setStyleSheet(
            "background: transparent; border: 1px solid " +
            Ui::stylesheetValues(theme)["EDIT_COLOR"]);
    mUi->bug_sendToGoogle->setIcon(getIconForCurrentTheme("open_in_browser"));
}

void BugreportPage::showEvent(QShowEvent* event) {
    // Override stylesheet for QPlainTextEdit[readOnly=true]
    SettingsTheme theme = getSelectedTheme();
    mUi->bug_bugReportTextEdit->setStyleSheet(
            "background: transparent; border: 1px solid " +
            Ui::stylesheetValues(theme)["EDIT_COLOR"]);

    QWidget::showEvent(event);
    // clean up the content loaded from last time.
    mUi->bug_bugReportTextEdit->clear();
    mUi->bug_reproStepsTextEdit->clear();
    mUi->bug_screenshotImage->clear();
    // check if save location is set
    if (mSavingStates.saveLocation.empty()) {
        mSavingStates.saveLocation = getScreenshotSaveDirectory().toStdString();
    }

    if (!mSavingStates.saveLocation.empty() &&
        System::get()->pathIsDir(mSavingStates.saveLocation) &&
        System::get()->pathCanWrite(mSavingStates.saveLocation)) {
        // Reset the bugreportSavedSucceed to avoid using stale bugreport
        // folder.
        mSavingStates.bugreportSavedSucceed = false;
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

void BugreportPage::saveBugReportFolder(bool willOpenIssueTracker) {
    QString dirName = QString::fromStdString(mSavingStates.saveLocation);
    dirName = QFileDialog::getExistingDirectory(
            Q_NULLPTR, tr("Report Saving Location"), dirName);

    if (dirName.isNull()) {
        return;
    } else {
        // launch the bugreport folder saving task in a separate thread
        std::string savingPath = PathUtils::join(
                dirName.toStdString(),
                AdbBugReportServices::generateUniqueBugreportName());
        std::string adbBugreportFilePath =
                (mUi->bug_bugReportCheckBox->isChecked() &&
                 mSavingStates.adbBugreportSucceed)
                        ? mSavingStates.adbBugreportFilePath
                        : "";
        std::string screenshotFilePath =
                (mUi->bug_screenshotCheckBox->isChecked() &&
                 mSavingStates.screenshotSucceed)
                        ? mSavingStates.screenshotFilePath
                        : "";
        QString reproSteps = mUi->bug_reproStepsTextEdit->toPlainText();
        reproSteps.truncate(2000);
        mReportingFields.reproSteps = reproSteps.toStdString();
        auto thread = new QThread();
        auto task = new BugReportFolderSavingTask(
                savingPath, adbBugreportFilePath, screenshotFilePath,
                mReportingFields.avdDetails, mReportingFields.reproSteps,
                willOpenIssueTracker);
        task->moveToThread(thread);
        connect(thread, SIGNAL(started()), task, SLOT(run()));
        connect(task, SIGNAL(started()), this,
                SLOT(saveBugreportFolderStarted()));
        connect(task, SIGNAL(finished(bool, QString, bool)), this,
                SLOT(saveBugreportFolderFinished(bool, QString, bool)));
        connect(task, SIGNAL(finished(bool, QString, bool)), thread,
                SLOT(quit()));
        connect(thread, SIGNAL(finished()), task, SLOT(deleteLater()));
        connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
        thread->start();
    }
}

void BugreportPage::saveBugreportFolderFinished(bool success,
                                                QString folderPath,
                                                bool willOpenIssueTracker) {
    SettingsTheme theme = getSelectedTheme();
    setButtonEnabled(mUi->bug_sendToGoogle, theme, true);
    setButtonEnabled(mUi->bug_saveButton, theme, true);
    mSavingStates.bugreportSavedSucceed = success;
    if (success) {
        mSavingStates.bugreportFolderPath = folderPath.toStdString();
        if (willOpenIssueTracker) {
            // launch the issue tracker in a separate thread
            launchIssueTrackerThread();
        }
    } else {
        showErrorDialog(tr("The bugreport save location is invalid.<br/>"
                           "Check the settings page and ensure the directory "
                           "exists and is writeable."),
                        tr("Bugreport"));
    }
}

void BugreportPage::saveBugreportFolderStarted() {
    SettingsTheme theme = getSelectedTheme();
    setButtonEnabled(mUi->bug_sendToGoogle, theme, false);
    setButtonEnabled(mUi->bug_saveButton, theme, false);
}

void BugreportPage::issueTrackerTaskFinished(bool success, QString error) {
    if (!success) {
        QString errMsg =
                tr("There was an error while opening the Google issue "
                   "tracker.<br/>");
        errMsg.append(error);
        showErrorDialog(errMsg, tr("File a Bug for Google"));
    }
}

void BugreportPage::on_bug_saveButton_clicked() {
    saveBugReportFolder(false);
}

void BugreportPage::on_bug_sendToGoogle_clicked() {
    if (!mSavingStates.bugreportSavedSucceed) {
        saveBugReportFolder(true);
    } else {
        QString reproSteps = mUi->bug_reproStepsTextEdit->toPlainText();
        reproSteps.truncate(2000);
        mReportingFields.reproSteps = reproSteps.toStdString();
        // launch the issue tracker in a separate thread
        launchIssueTrackerThread();
        QString dirName = QFileDialog::getOpenFileName(
                Q_NULLPTR, tr("Report Saving Location"),
                QString::fromStdString(mSavingStates.bugreportFolderPath));
    }
}

void BugreportPage::launchIssueTrackerThread() {
    auto thread = new QThread();
    auto task = new IssueTrackerTask(mReportingFields);
    task->moveToThread(thread);
    connect(thread, SIGNAL(started()), task, SLOT(run()));
    connect(task, SIGNAL(finished(bool, QString)), thread, SLOT(quit()));
    connect(task, SIGNAL(finished(bool, QString)), this,
            SLOT(issueTrackerTaskFinished(bool, QString)));
    connect(thread, SIGNAL(finished()), task, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();
}

void BugreportPage::loadAdbBugreport() {
    // Avoid generating adb bugreport multiple times while in execution.
    if (mBugReportServices->isBugReportInFlight())
        return;
    mSavingStates.adbBugreportSucceed = false;
    SettingsTheme theme = getSelectedTheme();
    setButtonEnabled(mUi->bug_sendToGoogle, theme, false);
    setButtonEnabled(mUi->bug_saveButton, theme, false);
    mUi->bug_circularSpinner->show();
    mUi->bug_collectingLabel->show();
    mBugReportServices->generateBugReport(
            System::get()->getTempDir(),
            [this, theme](AdbBugReportServices::Result result,
                          StringView filePath) {

                setButtonEnabled(mUi->bug_sendToGoogle, theme, true);
                setButtonEnabled(mUi->bug_saveButton, theme, true);
                mUi->bug_circularSpinner->hide();
                mUi->bug_collectingLabel->hide();
                if (System::get()->pathIsFile(
                            mSavingStates.adbBugreportFilePath)) {
                    System::get()->deleteFile(mSavingStates.adbBugreportFilePath);
                    mSavingStates.adbBugreportFilePath.clear();
                }
                if (result == AdbBugReportServices::Result::Success &&
                    System::get()->pathIsFile(filePath) &&
                    System::get()->pathCanRead(filePath)) {
                    mSavingStates.adbBugreportSucceed = true;
                    mSavingStates.adbBugreportFilePath = filePath;
                } else {
                    // TODO(wdu) Better error handling for failed adb bugreport
                    // generation
                    showErrorDialog(tr("There was an error while generating "
                                       "adb bugreport"),
                                    tr("Bugreport"));
                }
            });
}

void BugreportPage::loadAdbLogcat() {
    // Avoid generating adb logcat multiple times while in execution.
    if (mBugReportServices->isLogcatInFlight()) {
        return;
    }
    mBugReportServices->generateAdbLogcatInMemory(
            [this](AdbBugReportServices::Result result, StringView output) {
                if (result == AdbBugReportServices::Result::Success) {
                    mUi->bug_bugReportTextEdit->setPlainText(
                            QString::fromStdString(output));
                } else {
                    // TODO(wdu) Better error handling for failed adb logcat
                    // generation
                    mUi->bug_bugReportTextEdit->setPlainText(
                            tr("There was an error while loading adb logact"));
                }
            });
}

void BugreportPage::loadAvdDetails() {
    static std::vector<const char*> avdDetailsNoDisplayKeys{
            ABI_TYPE,    CPU_ARCH,    SKIN_NAME, SKIN_PATH,
            SDCARD_SIZE, SDCARD_PATH, IMAGES_2};

    CIniFile* configIni = avdInfo_getConfigIni(android_avdInfo);

    if (configIni) {
        mReportingFields.avdDetails.append(
                StringFormat("Name: %s\n", mReportingFields.deviceName));
        mReportingFields.avdDetails.append(StringFormat(
                "CPU/ABI: %s\n", avdInfo_getTargetCpuArch(android_avdInfo)));
        mReportingFields.avdDetails.append(StringFormat(
                "Path: %s\n", avdInfo_getContentPath(android_avdInfo)));
        mReportingFields.avdDetails.append(StringFormat(
                "Target: %s (API level %d)\n", avdInfo_getTag(android_avdInfo),
                avdInfo_getApiLevel(android_avdInfo)));
        char* skinName;
        char* skinDir;
        avdInfo_getSkinInfo(android_avdInfo, &skinName, &skinDir);
        if (skinName)
            mReportingFields.avdDetails.append(
                    StringFormat("Skin: %s\n", skinName));

        const char* sdcard = avdInfo_getSdCardSize(android_avdInfo);
        if (!sdcard) {
            sdcard = avdInfo_getSdCardPath(android_avdInfo);
        }
        mReportingFields.avdDetails.append(
                StringFormat("SD Card: %s\n", sdcard));

        if (iniFile_hasKey(configIni, SNAPSHOT_PRESENT)) {
            mReportingFields.avdDetails.append(StringFormat(
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
                    mReportingFields.avdDetails.append(
                            StringFormat("%s: %s\n", key, value));
                }
            }
        }
    }
}

void BugreportPage::loadScreenshotImage() {
    static const int MIN_SCREENSHOT_API = 14;
    static const int DEFAULT_API_LEVEL = 1000;
    // In case where the screencap doesn't work, bug reporting page should wait
    // for no longer 10s.
    static const int SCREEN_CAP_TIMEOUT_MS = 10000;
    mSavingStates.screenshotSucceed = false;
    int apiLevel = avdInfo_getApiLevel(android_avdInfo);
    if (apiLevel == DEFAULT_API_LEVEL || apiLevel < MIN_SCREENSHOT_API) {
        mUi->bug_screenshotImage->setText(
                tr("Screenshot is not supported below API 14."));
        return;
    }
    mUi->stackedWidget->setCurrentIndex(1);
    mScreenCapturer->capture(
            System::get()->getTempDir(),
            [this](ScreenCapturer::Result result, StringView filePath) {
                mUi->stackedWidget->setCurrentIndex(0);
                if (System::get()->pathIsFile(
                            mSavingStates.screenshotFilePath)) {
                    System::get()->deleteFile(mSavingStates.screenshotFilePath);
                    mSavingStates.screenshotFilePath.clear();
                }
                if (result == ScreenCapturer::Result::kSuccess &&
                    System::get()->pathIsFile(filePath) &&
                    System::get()->pathCanRead(filePath)) {
                    mSavingStates.screenshotSucceed = true;
                    mSavingStates.screenshotFilePath = filePath;
                    QPixmap image(filePath.c_str());

                    int height = mUi->bug_screenshotImage->height();
                    int width = mUi->bug_screenshotImage->width();
                    mUi->bug_screenshotImage->setPixmap(
                            image.scaled(width, height, Qt::KeepAspectRatio));
                } else if (result !=
                           ScreenCapturer::Result::kOperationInProgress) {
                    // TODO(wdu) Better error handling for failed screen capture
                    // operation
                    mUi->bug_screenshotImage->setText(
                            tr("There was an error while capturing the "
                               "screenshot."));
                }
            },
            SCREEN_CAP_TIMEOUT_MS);
}

bool BugreportPage::eventFilter(QObject* object, QEvent* event) {
    if (event->type() != QEvent::MouseButtonPress) {
        return false;
    }
    mDeviceDetailsDialog->show();
    return true;
}

BugReportFolderSavingTask::BugReportFolderSavingTask(
        std::string savingPath,
        std::string adbBugreportFilePath,
        std::string screenshotFilePath,
        std::string avdDetails,
        std::string reproSteps,
        bool openIssueTracker)
    : mSavingPath(savingPath),
      mAdbBugreportFilePath(adbBugreportFilePath),
      mScreenshotFilePath(screenshotFilePath),
      mAvdDetails(avdDetails),
      mReproSteps(reproSteps),
      mOpenIssueTracker(openIssueTracker) {}

void BugReportFolderSavingTask::run() {
    emit started();
    if (mSavingPath.empty()) {
        emit(finished(false, Q_NULLPTR, mOpenIssueTracker));
    }

    QString dirName = QString::fromStdString(mSavingPath);
    if (path_mkdir_if_needed(mSavingPath.c_str(), 0755)) {
        emit(finished(false, dirName, mOpenIssueTracker));
        return;
    }

    if (!mAdbBugreportFilePath.empty()) {
        std::string bugreportBaseName;
        if (PathUtils::extension(mAdbBugreportFilePath) == ".zip") {
            bugreportBaseName = "bugreport.zip";
        } else {
            bugreportBaseName = "bugreport.txt";
        }
        path_copy_file(PathUtils::join(mSavingPath, bugreportBaseName).c_str(),
                       mAdbBugreportFilePath.c_str());
    }

    if (!mScreenshotFilePath.empty()) {
        std::string screenshotBaseName = "screenshot.png";
        path_copy_file(PathUtils::join(mSavingPath, screenshotBaseName).c_str(),
                       mScreenshotFilePath.c_str());
    }

    if (!mAvdDetails.empty()) {
        auto avdDetailsFilePath =
                PathUtils::join(mSavingPath, "avd_details.txt");
        std::ofstream outFile(avdDetailsFilePath.c_str(),
                              std::ios_base::out | std::ios_base::trunc);
        outFile << mAvdDetails << std::endl;
    }

    if (!mReproSteps.empty()) {
        auto reproStepsFilePath =
                PathUtils::join(mSavingPath, "repro_steps.txt");
        std::ofstream outFile(reproStepsFilePath.c_str(),
                              std::ios_base::out | std::ios_base::trunc);
        outFile << mReproSteps << std::endl;
    }

    emit(finished(true, dirName, mOpenIssueTracker));
}

IssueTrackerTask::IssueTrackerTask(
        BugreportPage::ReportingFields reportingFields)
    : mReportingFields(reportingFields) {}

void IssueTrackerTask::run() {
    std::string bugTemplate = StringFormat(
            BUG_REPORT_TEMPLATE, mReportingFields.emulatorVer,
            mReportingFields.hypervisorVer, mReportingFields.sdkToolsVer,
            mReportingFields.hostOsName, trim(mReportingFields.cpuModel),
            mReportingFields.reproSteps);
    std::string unEncodedUrl =
            Uri::FormatEncodeArguments(FILE_BUG_URL, bugTemplate);
    QUrl url(QString::fromStdString(unEncodedUrl));
    if (!url.isValid() || !QDesktopServices::openUrl(url)) {
        emit finished(false, Q_NULLPTR);
    } else {
        emit finished(true, url.errorString());
    }
}
