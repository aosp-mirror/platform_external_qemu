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

#include "android/skin/qt/extended-pages/google-play-page.h"

#include "android/skin/qt/error-dialog.h"

#include <iostream>

using namespace android::base;
using android::emulation::GooglePlayServices;

GooglePlayPage::GooglePlayPage(QWidget* parent)
    : QWidget(parent),
      mGooglePlayServices(nullptr),
      mUi(new Ui::GooglePlayPage) {
    mUi->setupUi(this);
    QObject::connect(&mTimer, &QTimer::timeout, this,
                     &GooglePlayPage::getBootCompletionProperty);
    mTimer.setSingleShot(true);
    mTimer.setInterval(5000);  // 5 sec
}

GooglePlayPage::~GooglePlayPage() {}

void GooglePlayPage::initialize(android::emulation::AdbInterface* adb) {
    mGooglePlayServices.setAdbInterface(adb);
    mTimer.start();
}

void GooglePlayPage::getBootCompletionProperty() {
    // TODO: Really wish we had some kind of guest property to do
    // asynchronous waiting.
    const std::string boot_property = "sys.boot_completed";
    // Ran in a timer. We have to wait for the package manager
    // to start in order to query the versionName of the Play
    // Store and Play Services.
    mGooglePlayServices.getSystemProperty(
            boot_property,
            [this](GooglePlayServices::Result result, StringView outString) {
                GooglePlayPage::bootCompletionPropertyDone(result, outString);
            });
}

void GooglePlayPage::bootCompletionPropertyDone(
        GooglePlayServices::Result result,
        StringView outString) {
    if (result == GooglePlayServices::Result::kSuccess) {
        if ("1" == outString) {
            playStoreVersion();
            playServicesVersion();
        } else {
            // Continue to wait until it is finished booting.
            mTimer.start();
        }
    }
}

void GooglePlayPage::playStoreSettings() {
    mGooglePlayServices.showPlayStoreSettings(
            [this](GooglePlayServices::Result result) {
                GooglePlayPage::playStoreSettingsDone(result);
            });
}

void GooglePlayPage::playStoreSettingsDone(GooglePlayServices::Result result) {
    QString msg;
    switch (result) {
        case GooglePlayServices::Result::kSuccess:
            return;

        case GooglePlayServices::Result::kOperationInProgress:
            msg =
                    tr("Still waiting for Google Play Store to open.<br/>"
                       "Please try again later.");
            break;
        default:
            msg =
                    tr("There was an unknown error while opening Google "
                       "Play Store.");
    }
    showErrorDialog(msg, tr("Google Play Store Settings"));
}

void GooglePlayPage::playServicesPage() {
    mGooglePlayServices.showPlayServicesPage(
            [this](GooglePlayServices::Result result) {
                GooglePlayPage::playServicesPageDone(result);
            });
}

void GooglePlayPage::playServicesPageDone(GooglePlayServices::Result result) {
    QString msg;
    switch (result) {
        case GooglePlayServices::Result::kSuccess:
            return;

        case GooglePlayServices::Result::kOperationInProgress:
            msg =
                    tr("Still waiting for Google Play services page to open."
                       "<br/>Please try again later.");
            break;
        default:
            msg =
                    tr("There was an unknown error while opening Google "
                       "Play services page.");
    }
    showErrorDialog(msg, tr("Google Play services Page"));
}

void GooglePlayPage::playStoreVersion() {
    mGooglePlayServices.getPlayStoreVersion(
            [this](GooglePlayServices::Result result, StringView outString) {
                GooglePlayPage::playStoreVersionDone(result, outString);
            });
}

void GooglePlayPage::playStoreVersionDone(GooglePlayServices::Result result,
                                          StringView outString) {
    QString msg;
    switch (result) {
        case GooglePlayServices::Result::kSuccess:
            mUi->goog_playStoreVersionBox->setPlainText(
                    QString(outString.c_str()));
            return;

        case GooglePlayServices::Result::kAppNotInstalled:
            msg =
                    tr("It doesn't look like you have Google Play Store "
                       "installed.");
            break;
        case GooglePlayServices::Result::kOperationInProgress:
            msg =
                    tr("Still waiting for Google Play Store version.<br/>"
                       "Please try again later.");
            break;
        default:
            msg =
                    tr("There was an unknown error while getting Google "
                       "Play Store version.");
    }
    showErrorDialog(msg, tr("Google Play Store Version"));
}

void GooglePlayPage::playServicesVersion() {
    mGooglePlayServices.getPlayServicesVersion(
            [this](GooglePlayServices::Result result, StringView outString) {
                GooglePlayPage::playServicesVersionDone(result, outString);
            });
}

void GooglePlayPage::playServicesVersionDone(GooglePlayServices::Result result,
                                             StringView outString) {
    QString msg;
    switch (result) {
        case GooglePlayServices::Result::kSuccess:
            mUi->goog_playServicesVersionBox->setPlainText(
                    QString(outString.c_str()));
            return;

        case GooglePlayServices::Result::kAppNotInstalled:
            msg =
                    tr("It doesn't look like you have Google Play services "
                       "installed.");
            break;
        case GooglePlayServices::Result::kOperationInProgress:
            msg =
                    tr("Still waiting for Google Play services version.<br/>"
                       "Please try again later.");
            break;
        default:
            msg =
                    tr("There was an unknown error while getting the "
                       "Google Play services version.");
    }
    showErrorDialog(msg, tr("Google Play services Version"));
}

void GooglePlayPage::on_goog_updateServicesButton_clicked() {
    playServicesPage();
}

void GooglePlayPage::on_goog_updateStoreButton_clicked() {
    playStoreSettings();
}
