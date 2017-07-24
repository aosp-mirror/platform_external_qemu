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

#include "android/base/StringFormat.h"
#include "android/base/Uri.h"
#include "android/base/misc/StringUtils.h"
#include "android/base/system/System.h"
#include "android/bugreport_data.h"
#include "android/emulation/control/bugreport_agent.h"
#include "android/skin/qt/error-dialog.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/stylesheet.h"
#include "ui_bug-report-page.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QFontMetrics>
#include <QMovie>
#include <QObject>
#include <QUrl>

#include <algorithm>
#include <fstream>
#include <vector>

using android::base::StringFormat;
using android::base::System;
using android::base::Uri;
using android::base::trim;

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
    mDeviceDetailsDialog.setIcon(QMessageBox::NoIcon);
    mDeviceDetailsDialog.setStandardButtons(QMessageBox::Close);
    mDeviceDetailsDialog.setModal(false);
    mUi->bug_deviceLabel->installEventFilter(this);
    updateTheme();
}

void BugreportPage::setBugreportAgent(const QBugreportAgent* agent) {
    mBugreportAgent = agent;
}

void BugreportPage::bugreportDataCallback(BugreportData* data,
                                          BugreportDataType type,
                                          void* context) {
    BugreportPage* bugreportPage = (BugreportPage*)context;
    if (bugreportPage != NULL) {
        if (data->bugreportReady) {
            setButtonEnabled(bugreportPage->mUi->bug_sendToGoogle,
                             bugreportPage->mTheme, true);
            setButtonEnabled(bugreportPage->mUi->bug_saveButton,
                             bugreportPage->mTheme, true);
        }

        if (type == BugreportDataType::AdbBugreport) {
            bugreportPage->onReceiveAdbBugreport(data);
        } else if (type == AdbLogcat) {
            bugreportPage->onReceiveAdbLogcat(data);
        } else if (type == Screenshot) {
            bugreportPage->onReceiveScreenshot(data);
        } else if (type == AvdDetails) {
            bugreportPage->onReceiveAvdDetails(data);
        }
    }
}

void BugreportPage::updateTheme() {
    mTheme = getSelectedTheme();
    QMovie* movie = new QMovie(this);
    movie->setFileName(":/" + Ui::stylesheetValues(mTheme)[Ui::THEME_PATH_VAR] +
                       "/circular_spinner");
    if (movie->isValid()) {
        movie->start();
        mUi->bug_circularSpinner->setMovie(movie);
        mUi->bug_circularSpinner2->setMovie(movie);
    }

    mUi->bug_bugReportTextEdit->setStyleSheet(
            "background: transparent; border: 1px solid " +
            Ui::stylesheetValues(mTheme)["EDIT_COLOR"]);
    mUi->bug_sendToGoogle->setIcon(getIconForCurrentTheme("open_in_browser"));
}

void BugreportPage::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    // clean up the content loaded from last time.
    mUi->bug_bugReportTextEdit->clear();
    mUi->bug_reproStepsTextEdit->clear();
    mUi->bug_screenshotImage->clear();

    mUi->stackedWidget->setCurrentIndex(1);
    mUi->bug_circularSpinner->show();
    mUi->bug_collectingLabel->show();
    setButtonEnabled(mUi->bug_sendToGoogle, mTheme, false);
    setButtonEnabled(mUi->bug_saveButton, mTheme, false);

    mBugreport = mBugreportAgent->generateBugreport(
            &BugreportPage::bugreportDataCallback, (void*)this);
}

void BugreportPage::saveReport() {
    if (!mBugreport->bugreportReady) {
        return;
    }

    setButtonEnabled(mUi->bug_sendToGoogle, mTheme, false);
    setButtonEnabled(mUi->bug_saveButton, mTheme, false);
    QString dirName = getScreenshotSaveDirectory();
    dirName = QFileDialog::getExistingDirectory(
            Q_NULLPTR, tr("Choose Saving Location"), dirName);

    if (!dirName.isNull()) {
        bool res = false;
        std::string savePath = dirName.toStdString();
        if (System::get()->pathIsDir(savePath) &&
            System::get()->pathCanWrite(savePath)) {
            mBugreportAgent->setSavePath(mBugreport, savePath.c_str());
            QString reproSteps = mUi->bug_reproStepsTextEdit->toPlainText();
            reproSteps.truncate(2000);
            mBugreport->reproSteps = reproSteps.toStdString();
            int savingFlag = BugreportDataType::AvdDetails;
            if (mUi->bug_bugReportCheckBox->isChecked())
                savingFlag |= BugreportDataType::AdbBugreport;
            if (mUi->bug_screenshotCheckBox->isChecked())
                savingFlag |= BugreportDataType::Screenshot;
            res = (mBugreportAgent->saveBugreport(mBugreport, savingFlag) == 0)
                          ? true
                          : false;
        }

        if (!res) {
            showErrorDialog(
                    tr("The bugreport save location is invalid.<br/>"
                       "Check the settings page and ensure the directory "
                       "exists and is writeable."),
                    tr("Bugreport"));
        }
    }

    setButtonEnabled(mUi->bug_sendToGoogle, mTheme, true);
    setButtonEnabled(mUi->bug_saveButton, mTheme, true);
}

void BugreportPage::on_bug_saveButton_clicked() {
    saveReport();
}

void BugreportPage::on_bug_sendToGoogle_clicked() {
    if (!mBugreport->bugreportSavedSucceed) {
        saveReport();
    }
    if (!sendToGoogle()) {
        QString errMsg =
                tr("There was an error while opening the Google issue "
                   "tracker.<br/>");
        showErrorDialog(errMsg, tr("File a Bug for Google"));
    }
}

void BugreportPage::onReceiveAdbLogcat(const BugreportData* data) {
    if (data->adbLogactSucceed) {
        mUi->bug_bugReportTextEdit->setPlainText(
                QString::fromStdString(data->adbLogcatOutput));
    } else {
        // TODO(wdu) Better error handling for failed adb logcat
        // generation
        mUi->bug_bugReportTextEdit->setPlainText(
                tr("There was an error while loading adb logact"));
    }
}

void BugreportPage::onReceiveScreenshot(const BugreportData* data) {
    mUi->stackedWidget->setCurrentIndex(0);
    if (data->screenshotSucceed) {
        QPixmap image(data->screenshotFilePath.c_str());
        int height = mUi->bug_screenshotImage->height();
        int width = mUi->bug_screenshotImage->width();
        mUi->bug_screenshotImage->setPixmap(
                image.scaled(width, height, Qt::KeepAspectRatio));
    } else {
        // TODO(wdu) Better error handling for failed screen capture
        // operation
        mUi->bug_screenshotImage->setText(
                tr("There was an error while capturing the "
                   "screenshot."));
    }
}

void BugreportPage::onReceiveAdbBugreport(const BugreportData* data) {
    mUi->bug_circularSpinner->hide();
    mUi->bug_collectingLabel->hide();
    // TODO(wdu) Better error handling for failed adb bugreport
    // generation
    if (!data->adbBugreportSucceed) {
        showErrorDialog(tr("There was an error while generating "
                           "adb bugreport"),
                        tr("Bugreport"));
    }
}

void BugreportPage::onReceiveAvdDetails(const BugreportData* data) {
    mUi->bug_emulatorVersionLabel->setText(QString::fromStdString(
            StringFormat("%s (%s)", data->emulatorVer, data->hypervisorVer)));
    mUi->bug_androidVersionLabel->setText(
            QString::fromStdString(data->androidVer));

    QFontMetrics metrics(mUi->bug_deviceLabel->font());
    QString elidedText =
            metrics.elidedText(QString::fromStdString(data->deviceName),
                               Qt::ElideRight, mUi->bug_deviceLabel->width());
    mUi->bug_deviceLabel->setText(elidedText);
    mUi->bug_hostMachineLabel->setText(
            QString::fromStdString(data->hostOsName));
    mDeviceDetailsDialog.setWindowTitle(QString::fromStdString(
            StringFormat("Details for %s", data->deviceName)));
    mDeviceDetailsDialog.setInformativeText(
            QString::fromStdString(data->avdDetails));
}

bool BugreportPage::eventFilter(QObject* object, QEvent* event) {
    if (event->type() != QEvent::MouseButtonPress) {
        return false;
    }
    mDeviceDetailsDialog.show();
    return true;
}

bool BugreportPage::sendToGoogle() {
    std::string bugTemplate =
            StringFormat(BUG_REPORT_TEMPLATE, mBugreport->emulatorVer,
                         mBugreport->hypervisorVer, mBugreport->sdkToolsVer,
                         mBugreport->hostOsName, trim(mBugreport->cpuModel),
                         mBugreport->reproSteps);
    std::string unEncodedUrl =
            Uri::FormatEncodeArguments(FILE_BUG_URL, bugTemplate);
    QUrl url(QString::fromStdString(unEncodedUrl));
    bool res = url.isValid() && QDesktopServices::openUrl(url);
    if (mBugreport->bugreportSavedSucceed) {
        QFileDialog::getOpenFileName(
                Q_NULLPTR, QObject::tr("Report Saving Location"),
                QString::fromStdString(mBugreport->bugreportFolderPath));
    }
    return res;
};
