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

#include "android/avd/keys.h"
#include "android/base/StringFormat.h"
#include "android/base/files/PathUtils.h"
#include "android/emulation/ConfigDirs.h"
#include "android/emulation/CpuAccelerator.h"
#include "android/globals.h"
#include "android/skin/qt/error-dialog.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/qt-settings.h"
#include "android/skin/qt/stylesheet.h"
#include "android/update-check/VersionExtractor.h"
#include "android/utils/eintr_wrapper.h"
#include "android/utils/file_io.h"
#include "android/utils/path.h"
#include "ui_bug-report-window.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QMovie>
#include <QSettings>

#include <algorithm>
#include <fstream>
#include <vector>

using android::base::PathUtils;
using android::base::StringFormat;
using android::base::StringView;
using android::base::System;
using android::emulation::AdbInterface;
using android::emulation::AdbBugReportServices;
using android::emulation::ScreenCapturer;

static const char FILE_BUG_URL[] =
        "https://code.google.com/p/android/issues/"
        "entry?template=Android Emulator Bug&comment=%s";

static const char BUG_REPORT_TEMPLATE[] =
        "Please Read:\n"
        "http://tools.android.com/filing-bugs/emulator\n\n"
        "Emulator Version (Emulator--> Extended Controls--> Emulator "
        "Version):%s\n\n"
        "Android SDK Tools:%s\n\n"
        "Host Operating System:%s\n\n"
        "Steps to Reproduce Bug:\n\n"
        "%s\n\n"
        "Expected Behavior:\n\n\n\n"
        "Observed Behavior:\n\n";

BugReportWindow::BugReportWindow(EmulatorQtWindow* eW, QWidget* parent)
    : QFrame(parent),
      mEmulatorWindow(eW),
      mAdb(eW->getAdbInterface()),
      mBugReportServices(mAdb),
      mScreenCapturer(eW->getScreenCapturer()),
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

    mUi->bug_deviceLabel->installEventFilter(this);

    // Set emulator version and affix it with CPU accelerator version if
    // applicable
    android::update_check::VersionExtractor vEx;
    android::base::Version curVersion = vEx.getCurrentVersion();
    mEmulatorVer = curVersion.isValid() ? curVersion.toString() : "Unknown";
    android::CpuAccelerator accel = android::GetCurrentCpuAccelerator();
    if (accel != android::CPU_ACCELERATOR_NONE) {
        mEmulatorVer.append(StringFormat(" (%s %s)",
                                   android::CpuAcceleratorToString(accel),
                                   android::GetCurrentCpuAcceleratorVersion()));
    }
    mUi->bug_emulatorVersionLabel->setText(tr(mEmulatorVer.c_str()));

    // Set android guest system version
    char versionString[128];
    int apiLevel = avdInfo_getApiLevel(android_avdInfo);
    avdInfo_getFullApiName(apiLevel, versionString, 128);
    mUi->bug_androidVersionLabel->setText(QString(versionString));
    mAndroidVer = std::string(versionString);

    // Set AVD name
    const char* deviceName = avdInfo_getName(android_avdInfo);
    mDeviceName = std::string(deviceName);
    mUi->bug_deviceLabel->setText(QString(deviceName));

    // Set OS Description
    mHostOsName = android::base::toString(System::get()->getOsType());
    mUi->bug_hostMachineLabel->setText(QString(mHostOsName.c_str()));

    // The file at the $(ANDROID_SDK_ROOT)/tools/source.properties tells
    // what version the SDK Tools is.
    std::string sdkRootDirectory = android::ConfigDirs::getSdkRootDirectory();
    std::string propertiesPath =
            PathUtils::join(sdkRootDirectory, "tools", "source.properties");

    std::ifstream propertiesFile(propertiesPath.c_str());
    if (propertiesFile) {
        // Find the line contain "Pkg.Revision".
        std::string line;
        while (std::getline(propertiesFile, line)) {
            memset(versionString, '\0', 128);
            if (sscanf(line.c_str(), " Pkg.Revision = %s", versionString) >=
                1) {
                mSdkToolsVer.assign(versionString);
                break;
            }
        }
    }

    SettingsTheme theme = getSelectedTheme();
    QMovie* movie = new QMovie(this);
    movie->setFileName(":/" + Ui::stylesheetValues(theme)[Ui::THEME_PATH_VAR] +
                       "/circular_spinner");
    if (movie->isValid()) {
        movie->start();
        mUi->bug_circularSpinner->setMovie(movie);
        mUi->bug_circularSpinner->hide();
        mUi->bug_collectingLabel->hide();
        mUi->bug_circularSpinner2->setMovie(movie);
        mUi->bug_circularSpinner2->show();
    }

    loadAvdDetails();
    mDeviceDetailsDialog = new QMessageBox(
            QMessageBox::NoIcon,
            QString(StringFormat("Details for %s", deviceName).c_str()), "",
            QMessageBox::Close, this);
    mDeviceDetailsDialog->setModal(false);
    mDeviceDetailsDialog->setInformativeText(QString(mAvdDetails.c_str()));
}

void BugReportWindow::showEvent(QShowEvent* event) {
    // Align the left side of bugreport window with the extended window.
    if (mFirstShowEvent && !event->spontaneous()) {
        ToolWindow* tW = mEmulatorWindow->toolWindow();
        move(tW->geometry().right() + ToolWindow::toolGap,
             tW->geometry().top());
        mFirstShowEvent = false;
    }

    QWidget::showEvent(event);
    // clean up the content loaded from last time.
    mUi->bug_bugReportTextEdit->clear();
    mUi->bug_reproStepsTextEdit->clear();
    mUi->bug_screenshotImage->clear();
    // check if save location is set
    mSaveLocation = getScreenshotSaveDirectory().toStdString();
    if (!mSaveLocation.empty() && System::get()->pathIsDir(mSaveLocation) &&
        System::get()->pathCanWrite(mSaveLocation)) {
        // Whenever we reload adb bugreport and screenshot, we reset the
        // mBugreportSavedSucceed so as to avoid uploading stale bugreport.
        mBugreportSavedSucceed = false;
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

void BugReportWindow::saveBugReportFolder(const QObject* handler,
                                          const char* started_slot,
                                          const char* finished_slot,
                                          bool willOpenIssueTracker) {
    QString dirName = QString(mSaveLocation.c_str());
    dirName = QFileDialog::getExistingDirectory(
            Q_NULLPTR, tr("Save Report Location"), dirName);

    if (dirName.isNull()) {
        return;
    } else {
        BugReportFolderSavingThread* thread =
                BugReportFolderSavingThread::newInstance(
                        handler, started_slot ? started_slot : NULL,
                        finished_slot ? finished_slot : NULL);

        mSaveLocation = dirName.toStdString();
        thread->saveBugReportFromFiles(
                PathUtils::join(
                        mSaveLocation,
                        mBugReportServices.generateUniqueBugreportName()),
                (mUi->bug_bugReportCheckBox->isChecked() &&
                 mAdbBugreportSucceed)
                        ? mAdbBugreportFilePath
                        : "",
                (mUi->bug_screenshotCheckBox->isChecked() && mScreenshotSucceed)
                        ? mScreenshotFilePath
                        : "",
                mAvdDetails, willOpenIssueTracker);
    }
}

void BugReportWindow::saveBugreportFolderFinished(bool result,
                                                  QString folderPath,
                                                  bool willOpenIssueTracker) {
    SettingsTheme theme = getSelectedTheme();
    setButtonEnabled(mUi->bug_fileBugButton, theme, true);
    setButtonEnabled(mUi->bug_saveButton, theme, true);
    mBugreportSavedSucceed = result;
    if (result) {
        mBugreportFolderPath = folderPath.toStdString();
        if (willOpenIssueTracker) {
            IssueTrackerThread* thread = IssueTrackerThread::newInstance();
            thread->openIssueTracker(mReproSteps, mEmulatorVer, mSdkToolsVer,
                                     mHostOsName);
        }
    } else {
        showErrorDialog(tr("The bugreport save location is invalid.<br/>"
                           "Check the settings page and ensure the directory "
                           "exists and is writeable."),
                        tr("Bugreport"));
    }
}

void BugReportWindow::saveBugreportFolderStarted() {
    SettingsTheme theme = getSelectedTheme();
    setButtonEnabled(mUi->bug_fileBugButton, theme, false);
    setButtonEnabled(mUi->bug_saveButton, theme, false);
}

void BugReportWindow::on_bug_saveButton_clicked() {
    saveBugReportFolder(this, SLOT(saveBugreportFolderStarted()),
                        SLOT(saveBugreportFolderFinished(bool, QString, bool)),
                        false);
}

void BugReportWindow::on_bug_fileBugButton_clicked() {
    QString reproSteps = mUi->bug_reproStepsTextEdit->toPlainText();
    reproSteps.truncate(2000);
    mReproSteps = reproSteps.toStdString();
    if (!mBugreportSavedSucceed) {
        saveBugReportFolder(
                this, SLOT(saveBugreportFolderStarted()),
                SLOT(saveBugreportFolderFinished(bool, QString, bool)), true);

    } else {
        IssueTrackerThread* thread = IssueTrackerThread::newInstance();
        thread->openIssueTracker(mReproSteps, mEmulatorVer, mSdkToolsVer,
                                 mHostOsName);
        QString dirName = QFileDialog::getOpenFileName(
                Q_NULLPTR, tr("Bugreport Save location"),
                QString(mBugreportFolderPath.c_str()));
    }
}

void BugReportWindow::loadAdbBugreport() {
    // Avoid generating adb bugreport multiple times while in execution.
    if (mBugReportServices.isBugReportInFlight())
        return;
    mAdbBugreportSucceed = false;
    SettingsTheme theme = getSelectedTheme();
    setButtonEnabled(mUi->bug_fileBugButton, theme, false);
    setButtonEnabled(mUi->bug_saveButton, theme, false);
    mUi->bug_circularSpinner->show();
    mUi->bug_collectingLabel->show();
    mBugReportServices.generateBugReport(
            System::get()->getTempDir(),
            [this, theme](AdbBugReportServices::Result result,
                          StringView filePath) {
                setButtonEnabled(mUi->bug_fileBugButton, theme, true);
                setButtonEnabled(mUi->bug_saveButton, theme, true);
                mUi->bug_circularSpinner->hide();
                mUi->bug_collectingLabel->hide();
                if (result == AdbBugReportServices::Result::Success &&
                    System::get()->pathIsFile(filePath) &&
                    System::get()->pathCanRead(filePath)) {
                    mAdbBugreportSucceed = true;
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
    // Avoid generating adb logcat multiple times while in execution.
    if (mBugReportServices.isLogcatInFlight())
        return;
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

void BugReportWindow::loadAvdDetails() {
    static std::vector<const char*> avdDetailsNoDisplayKeys{
            ABI_TYPE,    CPU_ARCH,    SKIN_NAME, SKIN_PATH,
            SDCARD_SIZE, SDCARD_PATH, IMAGES_2};

    CIniFile* configIni = avdInfo_getConfigIni(android_avdInfo);

    if (configIni) {
        mAvdDetails.append(StringFormat("Name: %s\n", mDeviceName));
        mAvdDetails.append(StringFormat(
                "CPU/ABI: %s\n", avdInfo_getTargetCpuArch(android_avdInfo)));
        mAvdDetails.append(StringFormat(
                "Path: %s\n", avdInfo_getContentPath(android_avdInfo)));
        mAvdDetails.append(StringFormat("Target: %s (API level %d)\n",
                                        avdInfo_getTag(android_avdInfo),
                                        avdInfo_getApiLevel(android_avdInfo)));
        char* skinName;
        char* skinDir;
        avdInfo_getSkinInfo(android_avdInfo, &skinName, &skinDir);
        if (skinName)
            mAvdDetails.append(StringFormat("Skin: %s\n", skinName));

        const char* sdcard = avdInfo_getSdCardSize(android_avdInfo);
        if (!sdcard) {
            sdcard = avdInfo_getSdCardPath(android_avdInfo);
        }
        mAvdDetails.append(StringFormat("SD Card: %s\n", sdcard));

        if (iniFile_hasKey(configIni, SNAPSHOT_PRESENT)) {
            mAvdDetails.append(StringFormat(
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
                    mAvdDetails.append(StringFormat("%s: %s\n", key, value));
                }
            }
        }
    }
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
    mUi->stackedWidget->setCurrentIndex(1);
    mScreenCapturer->capture(
            System::get()->getTempDir(),
            [this](ScreenCapturer::Result result, StringView filePath) {
                mUi->stackedWidget->setCurrentIndex(0);
                if (result == ScreenCapturer::Result::kSuccess &&
                    System::get()->pathIsFile(filePath) &&
                    System::get()->pathCanRead(filePath)) {
                    mScreenshotSucceed = true;
                    mScreenshotFilePath = filePath;
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
            });
}

bool BugReportWindow::eventFilter(QObject* object, QEvent* event) {
    if (event->type() != QEvent::MouseButtonPress) {
        return false;
    }
    mDeviceDetailsDialog->show();
    return true;
}

BugReportFolderSavingThread* BugReportFolderSavingThread::newInstance(
        const QObject* handler,
        const char* started_slot,
        const char* finished_slot) {
    BugReportFolderSavingThread* new_instance =
            new BugReportFolderSavingThread();
    if (started_slot)
        connect(new_instance, SIGNAL(started()), handler, started_slot);
    if (finished_slot)
        connect(new_instance, SIGNAL(finished(bool, QString, bool)), handler,
                finished_slot);

    // Make sure new_instance gets cleaned up after the thread exits.
    connect(new_instance, &QThread::finished, new_instance,
            &QObject::deleteLater);

    return new_instance;
}

void BugReportFolderSavingThread::saveBugReportFromFiles(
        std::string savingLocation,
        std::string adbBugreportFilePath,
        std::string screenshotFilePath,
        std::string avdDetails,
        bool willOpenIssueTracker) {
    mSavingLocation = savingLocation;
    mAdbBugreportFilePath = adbBugreportFilePath;
    mScreenshotFilePath = screenshotFilePath;
    mAvdDetails = avdDetails;
    mOpenIssueTracker = willOpenIssueTracker;
    start();
}

void BugReportFolderSavingThread::run() {
    if (mSavingLocation.empty()) {
        emit(finished(false, nullptr, mOpenIssueTracker));
    }

    QString dirName = QString(mSavingLocation.c_str());
    if (path_mkdir_if_needed(mSavingLocation.c_str(), 0755)) {
        emit(finished(false, dirName, mOpenIssueTracker));
        return;
    }
    if (!mAdbBugreportFilePath.empty()) {
        std::string bugreportBaseName;
        if (mAdbBugreportFilePath.compare(
                    (mAdbBugreportFilePath.size() - 4 >= 0)
                            ? mAdbBugreportFilePath.size() - 4
                            : 0,
                    4, ".zip") == 0) {
            bugreportBaseName = "bugreport.zip";
        } else {
            bugreportBaseName = "bugreport.txt";
        }
        path_copy_file(
                PathUtils::join(mSavingLocation, bugreportBaseName).c_str(),
                mAdbBugreportFilePath.c_str());
    }
    if (!mScreenshotFilePath.empty()) {
        std::string screenshotBaseName = "screenshot.png";
        path_copy_file(
                PathUtils::join(mSavingLocation, screenshotBaseName).c_str(),
                mScreenshotFilePath.c_str());
    }

    if (!mAvdDetails.empty()) {
        auto avdDetailsFilePath =
                PathUtils::join(mSavingLocation, "avddetails.txt");
        std::ofstream outFile(avdDetailsFilePath.c_str(),
                              std::ios_base::out | std::ios_base::trunc);
        outFile << mAvdDetails << std::endl;
        outFile.close();
    }

    emit(finished(true, dirName, mOpenIssueTracker));
}

void IssueTrackerThread::openIssueTracker(std::string reproSteps,
                                          std::string emulatorVer,
                                          std::string sdkToolsVer,
                                          std::string hostOsName) {
    mReproSteps = reproSteps;
    mEmulatorVer = emulatorVer;
    mSdkToolsVer = sdkToolsVer;
    mHostOsName = hostOsName;
    start();
}

IssueTrackerThread* IssueTrackerThread::newInstance() {
    IssueTrackerThread* new_instance = new IssueTrackerThread();
    // Make sure new_instance gets cleaned up after the thread exits.
    connect(new_instance, &QThread::finished, new_instance,
            &QObject::deleteLater);
    return new_instance;
}

void IssueTrackerThread::run() {
    std::string comment = StringFormat(BUG_REPORT_TEMPLATE, mEmulatorVer,
                                       mSdkToolsVer, mHostOsName, mReproSteps);
    QString unencodedUrl = QString(StringFormat(FILE_BUG_URL, comment).c_str());
    QUrl url(unencodedUrl);
    if (url.isValid()) {
        QDesktopServices::openUrl(url);
    } else {
        QString errMsg =
                "There was an error while open the AOSP issue tracker.<br/>";
        errMsg.append(url.errorString());
        showErrorDialog(errMsg, tr("File a Bug for Google"));
    }
}
