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
#include "android/base/system/System.h"
#include "android/bugreport_data.h"
#include "android/emulation/control/bugreport_agent.h"
#include "android/skin/qt/error-dialog.h"
#include "android/skin/qt/stylesheet.h"
#include "ui_bug-report-page.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QFontMetrics>
#include <QMovie>

#include <algorithm>
#include <fstream>
#include <vector>

using android::base::StringFormat;
using android::base::System;

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

void BugreportPage::bugreportDataCallback(const BugreportData* data,
                                          BugreportDataType type,
                                          void* context) {
    BugreportPage* bugreportPage = (BugreportPage*)context;
    if (bugreportPage != NULL) {
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
    // reset saving state
    mBugreportSavedLocally = false;
    mBugreportAgent->generateBugreport(&BugreportPage::bugreportDataCallback,
                                       (void*)this);
}

void BugreportPage::saveReport() {
    setButtonEnabled(mUi->bug_sendToGoogle, mTheme, false);
    setButtonEnabled(mUi->bug_saveButton, mTheme, false);
    QString dirName = QString::fromStdString(mSaveDirectory);
    dirName = QFileDialog::getExistingDirectory(
            Q_NULLPTR, tr("Choose Saving Location"), dirName);

    if (!dirName.isNull()) {
        if (!mBugreportAgent->setSavePath(dirName.toStdString().c_str())) {
            QString reproSteps = mUi->bug_reproStepsTextEdit->toPlainText();
            reproSteps.truncate(2000);
            mBugreportAgent->setReproSteps(reproSteps.toStdString().c_str());
            int includeBugreport =
                    mUi->bug_bugReportCheckBox->isChecked() ? 1 : 0;
            int includeScreenshot =
                    mUi->bug_screenshotCheckBox->isChecked() ? 1 : 0;
            mBugreportSavedLocally =
                    (mBugreportAgent->saveBugreport(includeBugreport,
                                                    includeScreenshot) == 0)
                            ? true
                            : false;
        }

        if (!mBugreportSavedLocally) {
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
    if (!mBugreportSavedLocally) {
        saveReport();
    }
    if (mBugreportAgent->sendToGoogle()) {
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
    setButtonEnabled(mUi->bug_sendToGoogle, mTheme, true);
    setButtonEnabled(mUi->bug_saveButton, mTheme, true);
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
