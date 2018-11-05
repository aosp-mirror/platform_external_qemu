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
#include "android/base/files/IniFile.h"
#include "android/base/files/PathUtils.h"
#include "android/base/misc/StringUtils.h"
#include "android/crashreport/CrashReporter.h"
#include "android/crashreport/CrashSystem.h"
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
#include <QFuture>
#include <QMovie>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>
#include <fstream>

using android::base::IniFile;
using android::base::PathUtils;
using android::base::StringAppendFormat;
using android::base::StringFormat;
using android::base::StringView;
using android::base::System;
using android::base::trim;
using android::base::Uri;
using android::base::Version;
using android::crashreport::CrashReporter;
using android::crashreport::CrashService;
using android::crashreport::CrashSystem;
using android::emulation::AdbBugReportServices;
using android::emulation::AdbInterface;

static const char FILE_BUG_URL[] =
        "https://issuetracker.google.com/issues/new"
        "?component=192727&description=%s&template=843117";

static const std::string CrashURL = "https://clients2.google.com/cr/report";

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
    mReportingFields.androidVer = versionString;

    // Set AVD name
    mReportingFields.deviceName = StringView(avdInfo_getName(android_avdInfo));
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
    }

    loadAvdDetails();
    mDeviceDetailsDialog = new QMessageBox(
            QMessageBox::NoIcon,
            QString::fromStdString(StringFormat("Details for %s",
                                                mReportingFields.deviceName)),
            "", QMessageBox::Close, this);
    mDeviceDetailsDialog->setModal(false);
    mDeviceDetailsDialog->setInformativeText(
            QString::fromStdString(mReportingFields.avdDetails));
    mUi->bug_sendToGoogle->setIcon(getIconForCurrentTheme("open_in_browser"));

    mSavingStates.crashReporterLocation =
        CrashReporter::get()->getDataExchangeDir();
    CrashService* service =
            CrashService::makeCrashService(
                    EMULATOR_VERSION_STRING, EMULATOR_BUILD_STRING,
                    mSavingStates.crashReporterLocation.c_str())
                    .release();
    service->shouldDeleteCrashDataOnExit(true);

    CrashSystem::get()->setCrashURL(CrashURL);
    mUploadDialog = new UploadDialog(this, service);
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
}

void BugreportPage::updateTheme() {
    SettingsTheme theme = getSelectedTheme();
    QMovie* movie = new QMovie(this);
    movie->setFileName(":/" + Ui::stylesheetValues(theme)[Ui::THEME_PATH_VAR] +
                       "/circular_spinner");
    if (movie->isValid()) {
        movie->start();
        mUi->bug_circularSpinner->setMovie(movie);
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
        loadAdbLogcat();
        loadAdbBugreport();
        loadScreenshotImage();
    } else {
        showErrorDialog(tr("The bugreport save location is not set.<br/>"
                           "Check the settings page and ensure the directory "
                           "exists and is writeable."),
                        tr("Bugreport"));
    }
}

void BugreportPage::enableInput(bool enabled) {
    SettingsTheme theme = getSelectedTheme();
    setButtonEnabled(mUi->bug_sendToGoogle, theme, enabled);
    setButtonEnabled(mUi->bug_saveButton, theme, enabled);
}

void BugreportPage::on_bug_saveButton_clicked() {
    QString dirName = QString::fromStdString(mSavingStates.saveLocation);
    dirName = QFileDialog::getExistingDirectory(
            Q_NULLPTR, tr("Report Saving Location"), dirName);
    if (dirName.isNull())
        return;
    std::string savingPath = PathUtils::join(
            dirName.toStdString(),
            AdbBugReportServices::generateUniqueBugreportName());

    enableInput(false);
    QFuture<bool> future = QtConcurrent::run(
            this, &BugreportPage::saveBugReportTo, savingPath);
    bool success = future.result();
    enableInput(true);
    if (!success) {
        showErrorDialog(tr("The bugreport save location is invalid.<br/>"
                           "Check the settings page and ensure the directory "
                           "exists and is writeable."),
                        tr("Bugreport"));
    }
}

void BugreportPage::on_bug_sendToGoogle_clicked() {
    QSettings settings;
    auto reportPreference =
            static_cast<Ui::Settings::CRASHREPORT_PREFERENCE_VALUE>(
                    settings.value(Ui::Settings::CRASHREPORT_PREFERENCE, 0)
                            .toInt());

    // If the preference is never, launch issue tracker in a separate thread
    if (reportPreference == Ui::Settings::CRASHREPORT_PREFERENCE_NEVER) {
        QFuture<bool> future =
                QtConcurrent::run(this, &BugreportPage::launchIssueTracker);
        bool success = future.result();
        if (!success) {
            QString errMsg =
                    tr("There was an error while opening the Google issue "
                       "tracker.<br/>");
            // errMsg.append(error);
            showErrorDialog(errMsg, tr("File a Bug for Google"));
        }
        return;
    }

    mUploadDialog->exec();
    // If the preference is always, send directly to crash server
    if (reportPreference == Ui::Settings::CRASHREPORT_PREFERENCE_ALWAYS) {
        mUploadDialog->sendBugReport();
    }
}

bool BugreportPage::saveBugReportTo(const std::string& savingPath) {
    if (savingPath.empty()) {
        return false;
    }

    if (path_mkdir_if_needed(savingPath.c_str(), 0755)) {
        return false;
    }

    if (mUi->bug_bugReportCheckBox->isChecked() &&
        mSavingStates.adbBugreportSucceed) {
        StringView bugreportBaseName;
        if (PathUtils::extension(mSavingStates.adbBugreportFilePath) ==
            ".zip") {
            bugreportBaseName = "bugreport.zip";
        } else {
            bugreportBaseName = "bugreport.txt";
        }
        path_copy_file(PathUtils::join(savingPath, bugreportBaseName).c_str(),
                       mSavingStates.adbBugreportFilePath.c_str());
    }

    if (mUi->bug_screenshotCheckBox->isChecked() &&
        mSavingStates.screenshotSucceed) {
        StringView screenshotBaseName = "screenshot.png";
        path_copy_file(PathUtils::join(savingPath, screenshotBaseName).c_str(),
                       mSavingStates.screenshotFilePath.c_str());
    }

    if (!mReportingFields.avdDetails.empty()) {
        auto avdDetailsFilePath =
                PathUtils::join(savingPath, "avd_details.txt");
        std::ofstream outFile(avdDetailsFilePath.c_str(),
                              std::ios_base::out | std::ios_base::trunc);
        outFile << mReportingFields.avdDetails << std::endl;
    }

    QString reproSteps = mUi->bug_reproStepsTextEdit->toPlainText();
    reproSteps.truncate(2000);
    mReportingFields.reproSteps = reproSteps.toStdString();
    if (!mReportingFields.reproSteps.empty()) {
        auto reproStepsFilePath =
                PathUtils::join(savingPath, "repro_steps.txt");
        std::ofstream outFile(reproStepsFilePath.c_str(),
                              std::ios_base::out | std::ios_base::trunc);
        outFile << mReportingFields.reproSteps << std::endl;
    }
    return true;
}

bool BugreportPage::launchIssueTracker() {
    QString reproSteps = mUi->bug_reproStepsTextEdit->toPlainText();
    reproSteps.truncate(2000);
    mReportingFields.reproSteps = reproSteps.toStdString();
    // launch the issue tracker in a separate thread
    std::string bugTemplate = StringFormat(
            BUG_REPORT_TEMPLATE, mReportingFields.emulatorVer,
            mReportingFields.hypervisorVer, mReportingFields.sdkToolsVer,
            mReportingFields.hostOsName, trim(mReportingFields.cpuModel),
            mReportingFields.reproSteps);
    std::string unEncodedUrl =
            Uri::FormatEncodeArguments(FILE_BUG_URL, bugTemplate);
    QUrl url(QString::fromStdString(unEncodedUrl));
    if (!url.isValid() || !QDesktopServices::openUrl(url)) {
        return false;
    } else {
        return true;
    }
}

void BugreportPage::loadAdbBugreport() {
    // Avoid generating adb bugreport multiple times while in execution.
    if (mAdbBugreport && mAdbBugreport->inFlight())
        return;

    mSavingStates.adbBugreportSucceed = false;
    enableInput(false);
    mUi->bug_circularSpinner->show();
    mUi->bug_collectingLabel->show();
    mAdbBugreport = mBugReportServices->generateBugReport(
            System::get()->getTempDir(),
            [this](AdbBugReportServices::Result result, StringView filePath) {
                enableInput(true);
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
                    showErrorDialog(tr("Bug report interrupted by snapshot load? "
                                       "There was an error while generating "
                                       "adb bugreport"),
                                    tr("Bugreport"));
                }
                mAdbBugreport.reset();
            });
}

void BugreportPage::loadAdbLogcat() {
    // Avoid generating adb logcat multiple times while in execution.
    if (mAdbLogcat && mAdbLogcat->inFlight()) {
        return;
    }

    mAdbLogcat = mBugReportServices->generateAdbLogcatInMemory(
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
                mAdbLogcat.reset();
            });
}

void BugreportPage::loadAvdDetails() {
    static constexpr StringView kAvdDetailsNoDisplayKeys[] = {
            ABI_TYPE,    CPU_ARCH,    SKIN_NAME, SKIN_PATH,
            SDCARD_SIZE, SDCARD_PATH, IMAGES_2};

    if (const auto configIni = reinterpret_cast<const IniFile*>(
                avdInfo_getConfigIni(android_avdInfo))) {
        StringAppendFormat(&mReportingFields.avdDetails, "Name: %s\n",
                           mReportingFields.deviceName);
        auto cpuArch = avdInfo_getTargetCpuArch(android_avdInfo);
        StringAppendFormat(&mReportingFields.avdDetails, "CPU/ABI: %s\n",
                           cpuArch);
        free(cpuArch);
        StringAppendFormat(&mReportingFields.avdDetails, "Path: %s\n",
                           avdInfo_getContentPath(android_avdInfo));
        auto tag = avdInfo_getTag(android_avdInfo);
        StringAppendFormat(&mReportingFields.avdDetails,
                           "Target: %s (API level %d)\n", tag,
                           avdInfo_getApiLevel(android_avdInfo));
        free((char*)tag);

        char* skinName = nullptr;
        char* skinDir = nullptr;
        avdInfo_getSkinInfo(android_avdInfo, &skinName, &skinDir);
        if (skinName) {
            StringAppendFormat(&mReportingFields.avdDetails, "Skin: %s\n",
                               skinName);
            free(skinName);
        }
        free(skinDir);

        const char* sdcard = avdInfo_getSdCardSize(android_avdInfo);
        if (!sdcard) {
            sdcard = avdInfo_getSdCardPath(android_avdInfo);
        }
        if (!sdcard || !*sdcard) {
            sdcard = android::base::strDup("<none>");
        }
        StringAppendFormat(&mReportingFields.avdDetails, "SD Card: %s\n",
                           sdcard);
        free((char*)sdcard);

        if (configIni->hasKey(SNAPSHOT_PRESENT)) {
            StringAppendFormat(
                    &mReportingFields.avdDetails, "Snapshot: %s",
                    avdInfo_getSnapshotPresent(android_avdInfo) ? "yes" : "no");
        }

        for (const std::string& key : *configIni) {
            const std::string& value = configIni->getString(key, "");
            // check if the key is already displayed
            if (std::find_if(std::begin(kAvdDetailsNoDisplayKeys),
                             std::end(kAvdDetailsNoDisplayKeys),
                             [&key](StringView noDisplayKey) {
                                 return noDisplayKey == key;
                             }) == std::end(kAvdDetailsNoDisplayKeys)) {
                StringAppendFormat(&mReportingFields.avdDetails, "%s: %s\n",
                                   key, value);
            }
        }
    }
}

void BugreportPage::loadScreenshotImage() {
    // Delete previously taken screenshot if it exists
    if (System::get()->pathIsFile(mSavingStates.screenshotFilePath)) {
        System::get()->deleteFile(mSavingStates.screenshotFilePath);
        mSavingStates.screenshotFilePath.clear();
    }
    mSavingStates.screenshotSucceed = false;
    if (android::emulation::captureScreenshot(
                System::get()->getTempDir(),
                &mSavingStates.screenshotFilePath)) {
        if (System::get()->pathIsFile(mSavingStates.screenshotFilePath) &&
            System::get()->pathCanRead(mSavingStates.screenshotFilePath)) {
            mSavingStates.screenshotSucceed = true;
            QPixmap image(mSavingStates.screenshotFilePath.c_str());
            int height = mUi->bug_screenshotImage->height();
            int width = mUi->bug_screenshotImage->width();
            mUi->bug_screenshotImage->setPixmap(
                    image.scaled(width, height, Qt::KeepAspectRatio));
        }
    } else {
        // TODO(wdu) Better error handling for failed screen capture
        // operation
        mUi->bug_screenshotImage->setText(
                tr("There was an error while capturing the "
                   "screenshot."));
    }
}

bool BugreportPage::eventFilter(QObject* object, QEvent* event) {
    if (event->type() != QEvent::MouseButtonPress) {
        return false;
    }
    mDeviceDetailsDialog->show();
    return true;
}

UploadDialog::UploadDialog(QWidget* parent, CrashService* service)
    : QDialog(parent) {
    mCrashService.reset(service);
    QVBoxLayout* dialogLayout = new QVBoxLayout(this);
    mDialogLabel = new QLabel(tr(SEND_TO_GOOGLE));
    dialogLayout->addWidget(mDialogLabel);
    mButtonBox = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal);
    dialogLayout->addWidget(mButtonBox);
    connect(mButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(mButtonBox, SIGNAL(accepted()), this, SLOT(sendBugReport()));
    mProgressBar = new QProgressBar(this);
    mProgressBar->hide();
    mProgressBar->setMaximum(0);
    mProgressBar->setValue(-1);
    dialogLayout->addWidget(mProgressBar);

    setWindowTitle("Upload bug report");
    setLayout(dialogLayout);
    setModal(true);
}

void UploadDialog::collectSysInfo() {
    mDialogLabel->setText(COLLECT_INFO);
    mProgressBar->show();
    mButtonBox->hide();
    QFuture<bool> future = QtConcurrent::run(mCrashService.get(),
                                             &CrashService::collectSysInfo);
    future.waitForFinished();
}

void UploadDialog::sendBugReport() {
    collectSysInfo();
    mDialogLabel->setText(SENDING_REPORT);

    // Start the computation.
    QFuture<bool> future =
            QtConcurrent::run(mCrashService.get(), &CrashService::uploadCrash);
    mProgressBar->hide();

    bool success = future.result();
    if (success) {
        fprintf(stderr, "report upload succeed report id %s \n",
                mCrashService->getReportId().c_str());
    } else {
        fprintf(stderr, "report upload failed \n");
    }
    mDialogLabel->setText(SEND_TO_GOOGLE);
    mButtonBox->show();
    accept();
}