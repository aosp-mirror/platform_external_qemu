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

using android::base::StringView;
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

#define APP_TO_DESC(x, y) \
    { GooglePlayPage::PlayApps::x, y }
// static
const std::pair<GooglePlayPage::PlayApps, const char*>
        GooglePlayPage::PlayAppToDesc[] = {
                APP_TO_DESC(PlayStore, "Google Play Store"),
                APP_TO_DESC(PlayServices, "Google Play services"),
};
#undef APP_TO_DESC

GooglePlayPage::GooglePlayPage(QWidget* parent)
    : QWidget(parent), mUi(new Ui::GooglePlayPage) {
    mUi->setupUi(this);
}

GooglePlayPage::~GooglePlayPage() {}

void GooglePlayPage::initialize(
        android::emulation::AdbInterface* adb,
        android::emulation::AndroidPropertyInterface* androidProp) {
    mGooglePlayServices.reset(
            new android::emulation::GooglePlayServices(adb, androidProp));
    getBootCompletionProperty();
}

void GooglePlayPage::getBootCompletionProperty() {
    mGooglePlayServices->waitForBootCompletion(
            [this](GooglePlayServices::Result result, StringView outString) {
                GooglePlayPage::bootCompletionPropertyDone(result, outString);
            });
}

void GooglePlayPage::bootCompletionPropertyDone(
        GooglePlayServices::Result result,
        StringView outString) {
    if (result == GooglePlayServices::Result::Success) {
        getPlayStoreVersion();
        getPlayServicesVersion();
    } else {
        // We probably timed out. Let's try again.
        getBootCompletionProperty();
    }
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

QString GooglePlayPage::getPlayAppDescription(PlayApps app) {
    QString result;
    auto it = std::find_if(std::begin(PlayAppToDesc), std::end(PlayAppToDesc),
                           [app](const std::pair<PlayApps, QString>& value) {
                               return value.first == app;
                           });
    if (it != std::end(PlayAppToDesc)) {
        result = qApp->translate("PlayApps", it->second);
    }

    return result;
}

void GooglePlayPage::showPlayStoreSettings() {
    mGooglePlayServices->showPlayStoreSettings([this](
            GooglePlayServices::Result result) {
        GooglePlayPage::playPageDone(result, PlayPages::StoreSettingsPage);
    });
}

void GooglePlayPage::showPlayServicesPage() {
    mGooglePlayServices->showPlayServicesPage([this](
            GooglePlayServices::Result result) {
        GooglePlayPage::playPageDone(result, PlayPages::ServicesDetailsPage);
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

void GooglePlayPage::getPlayStoreVersion() {
    mGooglePlayServices->getPlayStoreVersion([this](
            GooglePlayServices::Result result, StringView outString) {
        GooglePlayPage::playVersionDone(result, PlayApps::PlayStore, outString);
    });
}

void GooglePlayPage::getPlayServicesVersion() {
    mGooglePlayServices->getPlayServicesVersion(
            [this](GooglePlayServices::Result result, StringView outString) {
                GooglePlayPage::playVersionDone(result, PlayApps::PlayServices,
                                                outString);
            });
}

void GooglePlayPage::playVersionDone(GooglePlayServices::Result result,
                                     PlayApps app,
                                     StringView outString) {
    QString msg;
    QPlainTextEdit* textEdit = nullptr;

    switch (app) {
        case PlayApps::PlayStore:
            textEdit = mUi->goog_playStoreVersionBox;
            notifyPlayStoreUpdate();
            break;
        case PlayApps::PlayServices:
            textEdit = mUi->goog_playServicesVersionBox;
            notifyPlayServicesUpdate();
            break;
        default:
            return;
    }

    switch (result) {
        case GooglePlayServices::Result::Success:
            textEdit->setPlainText(QString(outString.c_str()));
            return;

        case GooglePlayServices::Result::AppNotInstalled:
            textEdit->setPlainText(tr("Not Installed"));
            msg = tr("It doesn't look like you have %1 "
                     "installed.")
                          .arg(getPlayAppDescription(app));
            break;
        case GooglePlayServices::Result::OperationInProgress:
            textEdit->setPlainText(tr("Loading..."));
            msg = tr("Still waiting for %1 version.<br/>"
                     "Please try again later.")
                          .arg(getPlayAppDescription(app));
            break;
        default:
            textEdit->setPlainText(tr("Unknown Error"));
            msg = tr("There was an unknown error while getting %1"
                     " version.")
                          .arg(getPlayAppDescription(app));
    }
    showErrorDialog(msg, tr("%1 Version").arg(getPlayAppDescription(app)));
}

void GooglePlayPage::notifyPlayStoreUpdate() {
    mGooglePlayServices->waitForPlayStoreUpdate(
            [this](GooglePlayServices::Result result) {
                GooglePlayPage::updateNotification(result, PlayApps::PlayStore);
            });
}

void GooglePlayPage::notifyPlayServicesUpdate() {
    mGooglePlayServices->waitForPlayServicesUpdate(
            [this](GooglePlayServices::Result result) {
                GooglePlayPage::updateNotification(result,
                                                   PlayApps::PlayServices);
            });
}

void GooglePlayPage::updateNotification(GooglePlayServices::Result result,
                                        PlayApps app) {
    if (result == GooglePlayServices::Result::Success) {
        switch (app) {
            case PlayApps::PlayStore:
                getPlayStoreVersion();
                break;
            case PlayApps::PlayServices:
                getPlayServicesVersion();
                break;
        }
    } else {
        // Probably timed out. Let's wait again.
        switch (app) {
            case PlayApps::PlayStore:
                notifyPlayStoreUpdate();
                break;
            case PlayApps::PlayServices:
                notifyPlayServicesUpdate();
                break;
        }
    }
}

void GooglePlayPage::on_goog_updateServicesButton_clicked() {
    showPlayServicesPage();
}

void GooglePlayPage::on_goog_updateStoreButton_clicked() {
    showPlayStoreSettings();
}
