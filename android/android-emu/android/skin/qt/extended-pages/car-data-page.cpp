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
// TODO: (b/120444474) rename ERROR_INVALID_OPERATION & remove this, since it is
// defined in windows.h & redefined in VehicleHalProto.proto, causing conflicts.
#undef ERROR_INVALID_OPERATION
#include "android/emulation/proto/VehicleHalProto.pb.h"
#include "android/featurecontrol/feature_control.h"
#include "android/main-common.h"
#include "android/skin/qt/qt-settings.h"
#include "android/utils/debug.h"
#include "ui_car-data-page.h"

#include <QSettings>

#define D(...) VERBOSE_PRINT(car, __VA_ARGS__)

using std::string;
using emulator::EmulatorMessage;

const QCarDataAgent* CarDataPage::sCarDataAgent = nullptr;

CarDataPage::CarDataPage(QWidget* parent)
    : QWidget(parent), mUi(new Ui::CarDataPage) {
    mUi->setupUi(this);
    auto sendFunc = [this](const EmulatorMessage& msg, const string& log) {
        sendCarEmulatorMessageLogged(msg, log);
    };
    mUi->tab_sensor->setSendEmulatorMsgCallback(sendFunc);
    if (feature_is_enabled(kFeature_CarPropertyTable)) {
        mUi->car_property_table->setSendEmulatorMsgCallback(sendFunc);
    } else {
        mUi->tabWidget->removeTab(DATA_PROPERTY_TABLE_INDEX);
    }
    if (sCarDataAgent != nullptr) {
        sCarDataAgent->setCarCallback(&CarDataPage::carDataCallback, this);
    }
}

// static callback function wrapper
void CarDataPage::carDataCallback(const char* msg, int len, void* context) {
    CarDataPage* carDataPage = (CarDataPage*)context;
    if (carDataPage != nullptr) {
        carDataPage->onReceiveData(msg, len);
    }
}

/**
 * Callback to handle the data sent from vehicle hal.
 */
void CarDataPage::onReceiveData(const char* msg, int length) {
    // TODO: add more sophisticated processing.
    string protoStr(msg, length);
    EmulatorMessage emulatorMsg;
    string printMsg;
    if (emulatorMsg.ParseFromString(protoStr)) {
        printMsg = "Message Type: " + std::to_string(emulatorMsg.msg_type());
        printMsg += " Status: " + std::to_string(emulatorMsg.status());
        if (emulatorMsg.prop_size() > 0) {
            printMsg += " Prop: " + std::to_string(emulatorMsg.prop(0).prop());
        }
    } else {
        printMsg = "Received raw string: " + protoStr;
    }
    mUi->car_property_table->processMsg(emulatorMsg);
    mUi->tab_sensor->processMsg(emulatorMsg);
    D(printMsg.c_str());
}

// static
void CarDataPage::setCarDataAgent(const QCarDataAgent* agent) {
    if (agent == nullptr) D("data agent null");
    sCarDataAgent = agent;
}

void CarDataPage::sendCarEmulatorMessageLogged(const EmulatorMessage& msg,
                                               const string& log) {
    if (sCarDataAgent == nullptr) {
        return;
    }
    D(log.c_str());
    string msgString;
    if (msg.SerializeToString(&msgString)) {
        sCarDataAgent->sendCarData(msgString.c_str(), msgString.length());
    } else {
        D("Failed to send emulator message.");
    }
}
