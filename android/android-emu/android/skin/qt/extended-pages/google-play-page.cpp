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

#include "android/base/files/PathUtils.h"
#include "android/skin/qt/emulator-qt-window.h"        // for EmulatorQtWindow

#include <qapplication.h>                  // for QApplication (ptr only), qApp
#include <QApplication>                    // for QApplication
#include <QObject>                         // for QObject
#include <QPlainTextEdit>                  // for QPlainTextEdit
#include <algorithm>                       // for find_if
#include <functional>                      // for __base
#include <iterator>                        // for end, begin
#include <string_view>

#include "android/skin/qt/error-dialog.h"  // for showErrorDialog
#include "ui_google-play-page.h"           // for GooglePlayPage

class QPlainTextEdit;
class QString;
class QWidget;
namespace android {
namespace emulation {
class AdbInterface;
}  // namespace emulation
}  // namespace android

using android::base::c_str;
using android::emulation::GooglePlayServices;


#define PAGE_TO_DESC(x, y) \
    { GooglePlayPage::PlayPages::x, y }
// static
const std::pair<GooglePlayPage::PlayPages, const char*>
        GooglePlayPage::PlayPageToDesc[] = {
                PAGE_TO_DESC(ServicesDetailsPage, "Google Play services Page"),
};
#undef PAGE_TO_DESC

#define APP_TO_DESC(x, y) \
    { GooglePlayPage::PlayApps::x, y }
// static
const std::pair<GooglePlayPage::PlayApps, const char*>
        GooglePlayPage::PlayAppToDesc[] = {
                APP_TO_DESC(PlayServices, "Google Play services"),
};
#undef APP_TO_DESC

GooglePlayPage::GooglePlayPage(QWidget* parent)
    : QWidget(parent), mUi(new Ui::GooglePlayPage) {
    mUi->setupUi(this);
    QObject::connect(&mTimer, &QTimer::timeout, this,
                     &GooglePlayPage::getBootCompletionProperty);
    mTimer.setSingleShot(true);
    mTimer.setInterval(5000);  // 5 sec
}

GooglePlayPage::~GooglePlayPage() {}

void GooglePlayPage::initialize(android::emulation::AdbInterface* adb) {
    mGooglePlayServices.reset(new android::emulation::GooglePlayServices(adb));
    mTimer.start();
}

void GooglePlayPage::getBootCompletionProperty() {
    // TODO: Really wish we had some kind of guest property to do
    // asynchronous waiting.
    static constexpr std::string_view boot_property = "sys.boot_completed";
    // Ran in a timer. We have to wait for the package manager
    // to start in order to query the versionName of the Play
    // Store and Play Services.
    mGooglePlayServices->getSystemProperty(
            boot_property, [this](GooglePlayServices::Result result,
                                  std::string_view outString) {
                GooglePlayPage::bootCompletionPropertyDone(result, outString);
            });
}

void GooglePlayPage::queryPlayVersions() {
    getPlayServicesVersion();
}

void GooglePlayPage::bootCompletionPropertyDone(
        GooglePlayServices::Result result,
        std::string_view outString) {
    EmulatorQtWindow::getInstance()->runOnUiThread([this, result, outString] {
        if (result == GooglePlayServices::Result::Success && !outString.empty() &&
            outString[0] == '1') {
            // TODO: remove this once we have android properties to wait on.
            mTimer.disconnect();
            QObject::connect(&mTimer, &QTimer::timeout, this,
                             &GooglePlayPage::queryPlayVersions);
            mTimer.setSingleShot(false);
            mTimer.setInterval(10000); // 10 sec
            mTimer.start();
        } else {
            // Continue to wait until it is finished booting.
            mTimer.start();
        }
    });
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

void GooglePlayPage::showPlayServicesPage() {
    mGooglePlayServices->showPlayServicesPage([this](
            GooglePlayServices::Result result) {
        GooglePlayPage::playPageDone(result, PlayPages::ServicesDetailsPage);
    });
}

void GooglePlayPage::playPageDone(GooglePlayServices::Result result,
                                  PlayPages page) {
    EmulatorQtWindow::getInstance()->runOnUiThread([this, result, page] {
        QString msg;
        switch (result) {
            case GooglePlayServices::Result::Success:
            case GooglePlayServices::Result::OperationInProgress:
                return;
            default:
                msg = tr("There was an unknown error while opening %1.")
                              .arg(getPlayPageDescription(page));
        }
        showErrorDialog(msg, getPlayPageDescription(page));
    });
}

void GooglePlayPage::getPlayServicesVersion() {
    mGooglePlayServices->getPlayServicesVersion(
            [this](GooglePlayServices::Result result,
                   std::string_view outString) {
                GooglePlayPage::playVersionDone(result, PlayApps::PlayServices,
                                                outString);
            });
}

void GooglePlayPage::playVersionDone(GooglePlayServices::Result result,
                                     PlayApps app,
                                     std::string_view outString) {
    EmulatorQtWindow::getInstance()->runOnUiThread([this, result, app, outString] {
        QString msg;
        QPlainTextEdit* textEdit = nullptr;

        switch (app) {
            case PlayApps::PlayServices:
                textEdit = mUi->goog_playServicesVersionBox;
                break;
            default:
                return;
        }

        switch (result) {
            case GooglePlayServices::Result::Success:
                textEdit->setPlainText(QString::fromUtf8(outString.data(),
                                                         outString.size()));
                return;

            case GooglePlayServices::Result::AppNotInstalled:
                textEdit->setPlainText(tr("Not Installed"));
                break;
            case GooglePlayServices::Result::OperationInProgress:
                textEdit->setPlainText(tr("Loading..."));
                break;
            default:
                textEdit->setPlainText(tr("Unknown Error"));
        }
    });
}

void GooglePlayPage::on_goog_updateServicesButton_clicked() {
    showPlayServicesPage();
}
