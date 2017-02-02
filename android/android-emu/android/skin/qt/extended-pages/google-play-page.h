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

#include "android/emulation/control/GooglePlayServices.h"
#include "ui_google-play-page.h"

#include <QWidget>
#include <memory>

class GooglePlayPage : public QWidget {
    Q_OBJECT
public:
    explicit GooglePlayPage(QWidget* parent = 0);
    ~GooglePlayPage();

    void setAdbInterface(android::emulation::AdbInterface* adb);

private:
    void playStoreSettings();
    void playStoreSettingsDone(
            android::emulation::GooglePlayServices::Result result);
    void playServicesPage();
    void playServicesPageDone(
            android::emulation::GooglePlayServices::Result result);

private slots:
    void on_goog_updateServicesButton_clicked();
    void on_goog_updateStoreButton_clicked();

private:
    android::emulation::GooglePlayServices mGooglePlayServices;
    std::unique_ptr<Ui::GooglePlayPage> mUi;
};
