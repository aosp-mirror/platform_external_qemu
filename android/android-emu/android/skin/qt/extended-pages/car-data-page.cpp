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
#include "android/base/Log.h"
#include "android/skin/qt/qt-settings.h"
#include "ui_car-data-page.h"

#include <QSettings>


#include "android/utils/debug.h"

#define DINIT(...) do {  if (VERBOSE_CHECK(init)) dprint(__VA_ARGS__); } while (0)

CarDataPage::CarDataPage(QWidget *parent) :
    QWidget(parent),
    mUi(new Ui::CarDataPage)
{
    mUi->setupUi(this);

   // QObject::connect(this, &CarDataPage::requestUpdateData,
   //                      this, &CarDataPage::updateReceivedData);

    // Restore previous values
}


void onReceiveData(const char* msg) {
    DINIT("Yao callback.... %s ", msg);
   // emit requestUpdateData(msg);
}

void CarDataPage::updateReceivedData(const char *msg) {
    QString *str = new QString(msg);
    mUi->car_received_data->setPlainText(*str);
}

void CarDataPage::setCarDataAgent(const QCarDataAgent* agent) {
    DINIT("Yao: set data agent");

    if (agent == NULL) {
        DINIT("Yao: data agent null");

    } else {
        DINIT("Yao:  data agent not null");
        agent->setCallback(&onReceiveData);
    }
    mCarDataAgent = agent;
}

void CarDataPage::on_car_sendDataButton_clicked() {
    if (mCarDataAgent == NULL) {
        DINIT("Yao agent is null");

    } else {
        DINIT("Yao agent is not null");
        mCarDataAgent->sendCarData(mUi->car_send_data->toPlainText().toStdString().c_str());
    }
}