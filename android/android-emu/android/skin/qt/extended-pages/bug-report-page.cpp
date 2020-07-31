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

#include <QtConcurrent/qtconcurrentrun.h>              // for run
#include <QtCore/qglobal.h>                            // for Q_NULLPTR
#include <qcoreevent.h>                                // for QEvent (ptr only)
#include <qfuture.h>                                   // for QFutureInterfa...
#include <qmessagebox.h>                               // for QMessageBox::C...
#include <qnamespace.h>                                // for ElideRight
#include <qstring.h>                                   // for operator+, ope...
#include <time.h>                                      // for localtime, str...
#include <QCheckBox>                                   // for QCheckBox
#include <QDesktopServices>                            // for QDesktopServices
#include <QEvent>                                      // for QEvent
#include <QFileDialog>                                 // for QFileDialog
#include <QFontMetrics>                                // for QFontMetrics
#include <QFuture>                                     // for QFuture
#include <QHash>                                       // for QHash
#include <QLabel>                                      // for QLabel
#include <QMessageBox>                                 // for QMessageBox
#include <QMovie>                                      // for QMovie
#include <QPixmap>                                     // for QPixmap
#include <QPlainTextEdit>                              // for QPlainTextEdit
#include <QPushButton>                                 // for QPushButton
#include <QUrl>                                        // for QUrl
#include <fstream>                                     // for ofstream, ios_...
#include <functional>                                  // for __base
#include <iterator>                                    // for istreambuf_ite...
#include <vector>                                      // for vector

#include "android/avd/info.h"                          // for avdInfo_getApi...
#include "android/base/StringFormat.h"                 // for StringFormat
#include "android/base/Uri.h"                          // for Uri
#include "android/base/Uuid.h"                         // for Uuid
#include "android/base/files/PathUtils.h"              // for PathUtils
#include "android/base/system/System.h"                // for System, System...
#include "android/emulation/control/ScreenCapturer.h"  // for captureScreenshot
#include "android/globals.h"                           // for android_avdInfo
#include "android/skin/qt/error-dialog.h"              // for showErrorDialog
#include "android/skin/qt/emulator-qt-window.h"        // for runOnUiThread
#include "android/skin/qt/extended-pages/common.h"     // for getSelectedTheme
#include "android/skin/qt/raised-material-button.h"    // for RaisedMaterial...
#include "android/skin/qt/stylesheet.h"                // for stylesheetValues
#include "android/utils/path.h"                        // for path_copy_file
#include "ui_bug-report-page.h"                        // for BugreportPage

class QMovie;
class QObject;
class QShowEvent;
class QWidget;

using android::base::PathUtils;
using android::base::StringAppendFormat;
using android::base::StringFormat;
using android::base::StringView;
using android::base::System;
using android::base::Uri;
using android::base::Uuid;
using android::emulation::AdbInterface;
using android::emulation::OptionalAdbCommandResult;

static const int kDefaultUnknownAPILevel = 1000;
static const int kReproStepsCharacterLimit = 2000;
static const System::Duration kAdbCommandTimeoutMs = System::kInfinite;

static const char FILE_BUG_URL[] =
        "https://issuetracker.google.com/issues/new"
        "?component=192727&description=%s&template=843117";

// In reference to
// https://developer.android.com/studio/report-bugs.html#emulator-bugs
static const char BUG_REPORT_TEMPLATE[] =
        R"(Steps to Reproduce Bug:%s
Expected Behavior:

Observed Behavior:)";
BugreportPage::BugreportPage(QWidget* parent)
    : QWidget(parent), mUi(new Ui::BugreportPage) {
    mUi->setupUi(this);
    mUi->bug_deviceLabel->installEventFilter(this);
    mUi->bug_emulatorVersionLabel->setText(QString::fromStdString(
            StringFormat("%s (%s)", mReportingFields.emulatorVer,
                         mReportingFields.hypervisorVer)));
    mUi->bug_androidVersionLabel->setText(
            QString::fromStdString(mReportingFields.androidVer));
    QFontMetrics metrics(mUi->bug_deviceLabel->font());
    QString elidedText = metrics.elidedText(
            QString::fromStdString(mReportingFields.deviceName), Qt::ElideRight,
            mUi->bug_deviceLabel->width());
    mUi->bug_deviceLabel->setText(elidedText);
    mUi->bug_hostMachineLabel->setText(
            QString::fromStdString(mReportingFields.hostOsName));

    SettingsTheme theme = getSelectedTheme();
    QMovie* movie = new QMovie(this);
    movie->setFileName(":/" + Ui::stylesheetValues(theme)[Ui::THEME_PATH_VAR] +
                       "/circular_spinner");
    if (movie->isValid()) {
        movie->start();
        mUi->bug_circularSpinner->setMovie(movie);
    }

    mDeviceDetailsDialog = new QMessageBox(
            QMessageBox::NoIcon,
            QString::fromStdString(StringFormat("Details for %s",
                                                mReportingFields.deviceName)),
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

    // We need to cancel these running adb commands, as their result callbacks
    // may call runOnUiThread on an already destroyed EmulatorQtWindow instance.
    // Cancelling will ensure the result callback is not called.
    if (mAdbLogcat) {
        mAdbLogcat->cancel();
    }
    if (mAdbBugreport) {
        mAdbBugreport->cancel();
    }
}

void BugreportPage::setAdbInterface(AdbInterface* adb) {
    mAdb = adb;
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
        // Reset the bugreportSavedSucceed to avoid using stale bugreport
        // folder.
        mSavingStates.bugreportSavedSucceed = false;
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
    auto savingPath = PathUtils::join(dirName.toStdString(),
                                      generateUniqueBugreportName());

    enableInput(false);
    QFuture<bool> future = QtConcurrent::run(
            this, &BugreportPage::saveBugReportTo, savingPath);
    mSavingStates.bugreportSavedSucceed = future.result();
    enableInput(true);
    if (!mSavingStates.bugreportSavedSucceed) {
        showErrorDialog(tr("The bugreport save location is invalid.<br/>"
                           "Check the settings page and ensure the directory "
                           "exists and is writeable."),
                        tr("Bug Report"));
    }
}

void BugreportPage::on_bug_sendToGoogle_clicked() {
    if (!mSavingStates.bugreportSavedSucceed) {
        on_bug_saveButton_clicked();
    }
    // launch the issue tracker in a separate thread
    QFuture<bool> future =
            QtConcurrent::run(this, &BugreportPage::launchIssueTracker);
    bool success = future.result();
    if (!success) {
        QString errMsg =
                tr("There was an error while opening the Google issue "
                   "tracker.<br/>");
        showErrorDialog(errMsg, tr("File a Bug for Google"));
    }
}

bool BugreportPage::saveBugReportTo(StringView savingPath) {
    if (savingPath.empty()) {
        return false;
    }

    if (path_mkdir_if_needed(savingPath.data(), 0755)) {
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
        saveToFile(avdDetailsFilePath, mReportingFields.avdDetails.c_str(),
                   mReportingFields.avdDetails.length());
    }
    QString reproSteps = mUi->bug_reproStepsTextEdit->toPlainText();
    reproSteps.truncate(kReproStepsCharacterLimit);
    mReproSteps = reproSteps.toStdString();
    if (!mReproSteps.empty()) {
        auto reproStepsFilePath =
                PathUtils::join(savingPath, "repro_steps.txt");
        saveToFile(reproStepsFilePath, mReproSteps.c_str(),
                   mReproSteps.length());
    }
    return true;
}

bool BugreportPage::launchIssueTracker() {
    QString reproSteps = mUi->bug_reproStepsTextEdit->toPlainText();
    reproSteps.truncate(kReproStepsCharacterLimit);
    mReproSteps = reproSteps.toStdString();
    // launch the issue tracker in a separate thread
    std::string bugTemplate = mReportingFields.dump();

    StringAppendFormat(&bugTemplate, BUG_REPORT_TEMPLATE, mReproSteps);
    std::string encodedArgs =
            Uri::FormatEncodeArguments(FILE_BUG_URL, bugTemplate);
    QUrl url(QString::fromStdString(encodedArgs));
    return url.isValid() && QDesktopServices::openUrl(url);
}

void BugreportPage::loadAdbBugreport() {
    // Avoid generating adb bugreport multiple times while in execution.
    if (mAdbBugreport && mAdbBugreport->inFlight())
        return;

    mSavingStates.adbBugreportSucceed = false;
    enableInput(false);
    mUi->bug_circularSpinner->show();
    mUi->bug_collectingLabel->show();
    auto filePath = PathUtils::join(System::get()->getTempDir(),
                                    generateUniqueBugreportName());
    // In reference to
    // platform/frameworks/native/cmds/dumpstate/bugreport-format.md
    // Issue different command args given the API level

    int apiLevel = avdInfo_getApiLevel(android_avdInfo);
    bool isNougatOrHigher =
            (apiLevel != kDefaultUnknownAPILevel && apiLevel > 23);
    if (apiLevel == kDefaultUnknownAPILevel &&
        avdInfo_isMarshmallowOrHigher(android_avdInfo)) {
        isNougatOrHigher = true;
    }

    if (isNougatOrHigher)
        filePath.append(".zip");
    else
        filePath.append(".txt");
    bool wantOutput = !isNougatOrHigher;

    mAdbBugreport = mAdb->runAdbCommand(
            (isNougatOrHigher) ? std::vector<std::string>{"bugreport", filePath}
                               : std::vector<std::string>{"bugreport"},
            [this, filePath,
             wantOutput](const OptionalAdbCommandResult& result) {
                mAdbBugreport.reset();
                bool success = (result && result->exit_code == 0);
                if (success && wantOutput) {
                    std::string s(
                            std::istreambuf_iterator<char>(*result->output),
                            {});
                    success = saveToFile(filePath, s.c_str(), s.length());
                }


                EmulatorQtWindow::getInstance()->runOnUiThread([this, success, filePath] {
                    enableInput(true);
                    mUi->bug_circularSpinner->hide();
                    mUi->bug_collectingLabel->hide();
                    if (System::get()->pathIsFile(
                                mSavingStates.adbBugreportFilePath)) {
                        System::get()->deleteFile(
                                mSavingStates.adbBugreportFilePath);
                        mSavingStates.adbBugreportFilePath.clear();
                    }
                    if (success) {
                        mSavingStates.adbBugreportSucceed = true;
                        mSavingStates.adbBugreportFilePath = filePath;
                    } else {
                        // TODO(wdu) Better error handling for failed adb bugreport
                        // generation
                        showErrorDialog(
                                tr("Bug report interrupted by snapshot load? "
                                   "There was an error while generating "
                                   "adb bugreport"),
                                tr("Bug Report"));
                    }
                });
            },
            kAdbCommandTimeoutMs, wantOutput);
}

void BugreportPage::loadAdbLogcat() {
    // Avoid generating adb logcat multiple times while in execution.
    if (mAdbLogcat && mAdbLogcat->inFlight()) {
        return;
    }
    // After apiLevel 19, buffer "all" become available
    int apiLevel = avdInfo_getApiLevel(android_avdInfo);
    mAdbLogcat = mAdb->runAdbCommand(
            (apiLevel != kDefaultUnknownAPILevel && apiLevel > 19)
                    ? std::vector<std::string>{"logcat", "-b", "all", "-d"}
                    : std::vector<std::string>{"logcat", "-b", "events", "-b",
                                               "main", "-b", "radio", "-b",
                                               "system", "-d"},
            [this](const OptionalAdbCommandResult& result) {
                mAdbLogcat.reset();
                if (!result || result->exit_code || !result->output) {
                    EmulatorQtWindow::getInstance()->runOnUiThread([this] {
                        mUi->bug_bugReportTextEdit->setPlainText(
                                tr("There was an error while loading adb logcat"));
                    });
                } else {
                    std::string s(
                            std::istreambuf_iterator<char>(*result->output),
                            {});
                    EmulatorQtWindow::getInstance()->runOnUiThread([this, s] {
                        mUi->bug_bugReportTextEdit->setPlainText(
                                QString::fromStdString(s));
                    });
                }
            },
            kAdbCommandTimeoutMs, true);
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

bool BugreportPage::saveToFile(StringView filePath,
                               const char* content,
                               size_t length) {
    std::ofstream outFile(c_str(filePath),
                          std::ios_base::out | std::ios_base::binary);
    if (!outFile.is_open() || !outFile.good())
        return false;
    outFile.write(content, length);
    outFile.close();
    return true;
}

bool BugreportPage::eventFilter(QObject* object, QEvent* event) {
    if (event->type() != QEvent::MouseButtonPress) {
        return false;
    }
    mDeviceDetailsDialog->show();
    return true;
}

std::string BugreportPage::generateUniqueBugreportName() {
    const char* deviceName = avdInfo_getName(android_avdInfo);
    time_t now = System::get()->getUnixTime();
    char date[80];
    strftime(date, sizeof(date), "%Y-%m-%d-%H-%M-%S", localtime(&now));
    return StringFormat("bugreport-%s-%s-%s",
                        deviceName ? deviceName : "UNKNOWN_DEVICE", date,
                        Uuid::generate().toString());
}
