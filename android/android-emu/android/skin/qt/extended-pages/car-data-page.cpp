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

#include "android/emulation/proto/VehicleHalProto.pb.h"

#include <QSettings>


#include "android/utils/debug.h"

#define DINIT(...) do {  if (VERBOSE_CHECK(init)) dprint(__VA_ARGS__); } while (0)
using namespace std;
using namespace emulator;
CarDataPage::CarDataPage(QWidget *parent) :
    QWidget(parent),
    mUi(new Ui::CarDataPage)
{
    mUi->setupUi(this);

    QObject::connect(this, &CarDataPage::requestUpdateData,
                      this, &CarDataPage::updateReceivedData);
    // Restore previous values
}


void CarDataPage::onReceiveData(const char* msg, int length) {
    DINIT("Yao callback.... %s %d", msg, length);
    string protoStr(msg, length);
    EmulatorMessage* emulatorMsg = new EmulatorMessage();
    string receivedMsg;
        if (emulatorMsg->ParseFromString(protoStr)) {
            DINIT("msg type: %d, Status: %d , prop size %d", emulatorMsg->msg_type(), emulatorMsg->status(), emulatorMsg->prop_size());
            receivedMsg = "Message Type: " + std::to_string(emulatorMsg->msg_type());
            receivedMsg += " Status: " + std::to_string(emulatorMsg->status());
            if (emulatorMsg->prop_size() > 0) {
                receivedMsg += " Prop: " + std::to_string(emulatorMsg->prop(0).prop());
            }
        } else {
            receivedMsg = "Cannot parse from protobuf. " + protoStr;
        }


    QString qString = QString::fromStdString(receivedMsg);
    emit requestUpdateData(qString);
}

void CarDataPage::updateReceivedData(QString msg) {
    mUi->car_received_data->setPlainText(msg);
}

void CarDataPage::setCarDataAgent(const QCarDataAgent* agent) {
    DINIT("Yao: set data agent");

    if (agent == NULL) {
        DINIT("Yao: data agent null");

    } else {
        DINIT("Yao:  data agent not null");
        auto callback =
        std::bind(&CarDataPage::onReceiveData, this, std::placeholders::_1, std::placeholders::_2);
        agent->setCallback(callback);
    }
    mCarDataAgent = agent;
}

void CarDataPage::on_car_sendDataButton_clicked() {
    if (mCarDataAgent == NULL) {
        DINIT("Yao agent is null");
    } else {
        DINIT("Yao agent is not null");
        string msg = mUi->car_send_data->toPlainText().toStdString();
        mCarDataAgent->sendCarData(msg.c_str(), msg.length());
    }
}
