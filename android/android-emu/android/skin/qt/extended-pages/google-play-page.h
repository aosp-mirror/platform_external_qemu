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
#pragma once

#include <qobjectdefs.h>                                   // for Q_OBJECT
#include <QString>                                         // for QString
#include <QTimer>                                          // for QTimer
#include <QWidget>                                         // for QWidget
#include <memory>                                          // for unique_ptr
#include <utility>                                         // for pair

#include "android/base/StringView.h"                       // for StringView
#include "android/emulation/control/GooglePlayServices.h"  // for GooglePlay...

class QObject;
class QString;
class QWidget;
namespace Ui {
class GooglePlayPage;
}  // namespace Ui
namespace android {
namespace emulation {
class AdbInterface;
}  // namespace emulation
}  // namespace android

class GooglePlayPage : public QWidget {
    Q_OBJECT
public:
    enum class PlayPages {
        ServicesDetailsPage,
    };

    enum class PlayApps {
        PlayServices,
    };

    explicit GooglePlayPage(QWidget* parent = 0);
    ~GooglePlayPage();

    void initialize(android::emulation::AdbInterface* adb);

private:
    void showPlayServicesPage();
    void playPageDone(android::emulation::GooglePlayServices::Result result,
                      PlayPages page);
    static QString getPlayAppDescription(PlayApps app);
    static QString getPlayPageDescription(PlayPages page);
    void bootCompletionPropertyDone(
            android::emulation::GooglePlayServices::Result result,
            android::base::StringView outString);
    void getPlayServicesVersion();
    void playVersionDone(android::emulation::GooglePlayServices::Result result,
                         PlayApps app,
                         android::base::StringView outString);

private slots:
    void on_goog_updateServicesButton_clicked();

    void getBootCompletionProperty();
    void queryPlayVersions();

private:
    static const std::pair<PlayApps, const char*> PlayAppToDesc[];
    static const std::pair<PlayPages, const char*> PlayPageToDesc[];

    QTimer mTimer;
    std::unique_ptr<android::emulation::GooglePlayServices> mGooglePlayServices;
    std::unique_ptr<Ui::GooglePlayPage> mUi;
};
