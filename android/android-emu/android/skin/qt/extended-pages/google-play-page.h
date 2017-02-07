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

#include "android/base/StringView.h"
#include "android/emulation/control/GooglePlayServices.h"
#include "ui_google-play-page.h"

#include <QTimer>
#include <QWidget>
#include <memory>

class GooglePlayPage : public QWidget {
    Q_OBJECT
public:
    explicit GooglePlayPage(QWidget* parent = 0);
    ~GooglePlayPage();

    void initialize(android::emulation::AdbInterface* adb);

private:
    void bootCompletionPropertyDone(
            android::emulation::GooglePlayServices::Result result,
            android::base::StringView outString);
    void playStoreSettings();
    void playStoreSettingsDone(
            android::emulation::GooglePlayServices::Result result);
    void playServicesPage();
    void playServicesPageDone(
            android::emulation::GooglePlayServices::Result result);
    void playStoreVersion();
    void playStoreVersionDone(
            android::emulation::GooglePlayServices::Result result,
            android::base::StringView outString);
    void playServicesVersion();
    void playServicesVersionDone(
            android::emulation::GooglePlayServices::Result result,
            android::base::StringView outString);

private slots:
    void on_goog_updateServicesButton_clicked();
    void on_goog_updateStoreButton_clicked();

    void getBootCompletionProperty();

private:
    QTimer mTimer;
    android::emulation::GooglePlayServices mGooglePlayServices;
    std::unique_ptr<Ui::GooglePlayPage> mUi;
};
