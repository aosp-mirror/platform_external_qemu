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

using android::emulation::GooglePlayServices;

GooglePlayPage::GooglePlayPage(QWidget* parent)
    : QWidget(parent),
      mGooglePlayServices(nullptr),
      mUi(new Ui::GooglePlayPage) {
    mUi->setupUi(this);
}

GooglePlayPage::~GooglePlayPage() {}

void GooglePlayPage::setAdbInterface(android::emulation::AdbInterface* adb) {
    mGooglePlayServices.setAdbInterface(adb);
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
                    tr("Still waiting for play store to open.<br/>"
                       "Please try again later.");
            break;
        default:
            msg =
                    tr("There was an unknown error while opening the "
                       "play store.");
    }
    showErrorDialog(msg, tr("Play Store Settings"));
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
                    tr("Still waiting for play services page to open.<br/>"
                       "Please try again later.");
            break;
        default:
            msg =
                    tr("There was an unknown error while opening the "
                       "play services page.");
    }
    showErrorDialog(msg, tr("Play Services Page"));
}

void GooglePlayPage::on_goog_updateServicesButton_clicked() {
    playServicesPage();
}

void GooglePlayPage::on_goog_updateStoreButton_clicked() {
    playStoreSettings();
}
