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

#define PAGE_TO_DESC(x, y) \
    { GooglePlayPage::PlayPages::x, y }
// static
const std::pair<GooglePlayPage::PlayPages, const char*>
        GooglePlayPage::PlayPageToDesc[] = {
                PAGE_TO_DESC(StoreSettingsPage,
                             "Google Play Store Settings Page"),
                PAGE_TO_DESC(ServicesDetailsPage, "Google Play services Page"),
};
#undef PAGE_TO_DESC

GooglePlayPage::GooglePlayPage(QWidget* parent)
    : QWidget(parent), mUi(new Ui::GooglePlayPage) {
    mUi->setupUi(this);
}

GooglePlayPage::~GooglePlayPage() {}

void GooglePlayPage::setAdbInterface(android::emulation::AdbInterface* adb) {
    mGooglePlayServices.reset(new android::emulation::GooglePlayServices(adb));
}

QString GooglePlayPage::getPlayPageDescription(PlayPages page) {
    QString result;
    auto it = std::find_if(std::begin(PlayPageToDesc), std::end(PlayPageToDesc),
                           [page](const std::pair<PlayPages, QString>& value) {
                               return value.first == page;
                           });
    if (it != std::end(PlayPageToDesc)) {
        result = qApp->translate("PlayPages", it->second);
    }

    return result;
}

void GooglePlayPage::showPlayStoreSettings() {
    mGooglePlayServices->showPlayStoreSettings([this](
            GooglePlayServices::Result result) {
        GooglePlayPage::playPageDone(result, PlayPages::StoreSettingsPage);
    });
}

void GooglePlayPage::playPageDone(GooglePlayServices::Result result,
                                  PlayPages page) {
    QString msg;
    switch (result) {
        case GooglePlayServices::Result::Success:
            return;

        case GooglePlayServices::Result::OperationInProgress:
            msg = tr("Still waiting for %1 to open.<br/>"
                     "Please try again later.")
                          .arg(getPlayPageDescription(page));
            break;
        default:
            msg = tr("There was an unknown error while opening %1.")
                          .arg(getPlayPageDescription(page));
    }
    showErrorDialog(msg, getPlayPageDescription(page));
}

void GooglePlayPage::showPlayServicesPage() {
    mGooglePlayServices->showPlayServicesPage([this](
            GooglePlayServices::Result result) {
        GooglePlayPage::playPageDone(result, PlayPages::ServicesDetailsPage);
    });
}

void GooglePlayPage::on_goog_updateServicesButton_clicked() {
    showPlayServicesPage();
}

void GooglePlayPage::on_goog_updateStoreButton_clicked() {
    showPlayStoreSettings();
}
