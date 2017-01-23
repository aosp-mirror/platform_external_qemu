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
#include "android/skin/qt/extended-pages/car-data-page.h"

#include "android/emulation/control/car_data_agent.h"
#include "android/main-common.h"
#include "android/skin/qt/qt-settings.h"
#include "ui_car-data-page.h"

#include <QSettings>

CarDataPage::CarDataPage(QWidget *parent) :
    QWidget(parent),
    mUi(new Ui::CarDataPage())
{
    mUi->setupUi(this);

    // Restore previous values
}

void CarDataPage::setCarDataAgent(const QCarDataAgent* agent) {
    mCarDataAgent = agent;
}

void CarDataPage::on_car_sendDataButton_clicked() {
    mCarDataAgent->sendCarData("Hello");
}