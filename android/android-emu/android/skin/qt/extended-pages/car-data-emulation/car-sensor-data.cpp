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
#include "android/skin/qt/extended-pages/car-data-emulation/car-sensor-data.h"

#include "android/skin/qt/qt-settings.h"
#include "android/utils/debug.h"
#include "ui_car-sensor-data.h"
#include "vehicle_constants.h"

#include <QSettings>

#define D(...) VERBOSE_PRINT(car, __VA_ARGS__)

using std::string;

using emulator::EmulatorMessage;
using emulator::MsgType;
using emulator::Status;
using emulator::VehicleProperty;
using emulator::VehiclePropValue;
using emulator::VehicleGear;
using emulator::VehicleDrivingStatus;
using emulator::VehicleIgnitionState;

CarSensorData::CarSensorData(QWidget* parent)
    : QWidget(parent), mUi(new Ui::CarSensorData) {
    mUi->setupUi(this);
}

static EmulatorMessage makeSetPropMsg() {
    EmulatorMessage emulatorMsg;
    emulatorMsg.set_msg_type(MsgType::SET_PROPERTY_CMD);
    emulatorMsg.set_status(Status::RESULT_OK);
    return emulatorMsg;
}

void CarSensorData::sendGearChangeMsg(const int gear, const string& gearName) {
    // TODO: Grey out the buttons when callback is not set or vehicle hal is
    // not connected.
    if (mSendEmulatorMsg == nullptr) {
        return;
    }

    EmulatorMessage emulatorMsg = makeSetPropMsg();
    VehiclePropValue* value = emulatorMsg.add_value();
    value->set_prop(static_cast<int32_t>(VehicleProperty::CURRENT_GEAR));
    value->add_int32_values(gear);
    string log = "Gear changed to " + gearName;
    mSendEmulatorMsg(emulatorMsg, log);
}

void CarSensorData::sendDrivingStatusChangeMsg(const int status, const string& statusName) {
    if (mSendEmulatorMsg == nullptr) {
        return;
    }

    EmulatorMessage emulatorMsg = makeSetPropMsg();
    VehiclePropValue* value = emulatorMsg.add_value();
    value->set_prop(static_cast<int32_t>(VehicleProperty::DRIVING_STATUS));
    value->add_int32_values(status);
    string log = "Driving status: " + statusName;
    mSendEmulatorMsg(emulatorMsg, log);
}

void CarSensorData::collectDrivingStatusAndReport() {
    if (mSendEmulatorMsg == nullptr) {
        return;
    }
    string statusMsg;
    int drivingStatus = 0;
    if (mUi->checkBox_no_video->isChecked()) {
        drivingStatus |= static_cast<int32_t>(VehicleDrivingStatus::NO_VIDEO);
        statusMsg += "NO_VIDEO ";
    }

    if (mUi->checkBox_no_keyboard->isChecked()) {
        drivingStatus |=
                static_cast<int32_t>(VehicleDrivingStatus::NO_KEYBOARD_INPUT);
        statusMsg += "NO_KEYBOARD_INPUT ";
    }

    if (mUi->checkBox_limit_msg_len->isChecked()) {
        drivingStatus |=
                static_cast<int32_t>(VehicleDrivingStatus::LIMIT_MESSAGE_LEN);
        statusMsg += "LIMIT_MESSAGE_LEN ";
    }

    if (mUi->checkBox_no_config->isChecked()) {
        drivingStatus |= static_cast<int32_t>(VehicleDrivingStatus::NO_CONFIG);
        statusMsg += "NO_CONFIG ";
    }

    if (mUi->checkBox_no_voice->isChecked()) {
        drivingStatus |=
                static_cast<int32_t>(VehicleDrivingStatus::NO_VOICE_INPUT);
        statusMsg += "NO_VOICE_INPUT ";
    }

    if (drivingStatus != 0) {
        mUi->checkBox_unrestricted->setChecked(false);
    }

    sendDrivingStatusChangeMsg(drivingStatus, statusMsg);
}

void CarSensorData::sendIgnitionChangeMsg(const int ignition, const string& ignitionName) {
    if (mSendEmulatorMsg == nullptr) {
        return;
    }

    EmulatorMessage emulatorMsg = makeSetPropMsg();
    VehiclePropValue* value = emulatorMsg.add_value();
    value->set_prop(static_cast<int32_t>(VehicleProperty::IGNITION_STATE));
    value->add_int32_values(ignition);
    string log = "Ignition state: " + ignitionName;
    mSendEmulatorMsg(emulatorMsg, log);
}

void CarSensorData::on_car_speedSlider_valueChanged(int speed) {
    // TODO: read static configs from vehical Hal to determine what unit to use,
    // mph or kmph
    mUi->car_speedLabel->setText(QString::number(speed) + " mph");
    if (mSendEmulatorMsg != nullptr) {
        EmulatorMessage emulatorMsg = makeSetPropMsg();
        VehiclePropValue* value = emulatorMsg.add_value();
        value->set_prop(
                static_cast<int32_t>(VehicleProperty::PERF_VEHICLE_SPEED));
        value->add_float_values(speed);
        string log = "Speed changed to " + std::to_string(speed);
        mSendEmulatorMsg(emulatorMsg, log);
    }
}

void CarSensorData::setSendEmulatorMsgCallback(EmulatorMsgCallback&& func) {
    mSendEmulatorMsg = func;
}

void CarSensorData::on_radioButton_D_toggled() {
    if (mUi->radioButton_D->isChecked()) {
        string log("D");
        sendGearChangeMsg(static_cast<int32_t>(VehicleGear::GEAR_DRIVE), log);
    }
}

void CarSensorData::on_radioButton_R_toggled() {
    if (mUi->radioButton_R->isChecked()) {
        string log("R");
        sendGearChangeMsg(static_cast<int32_t>(VehicleGear::GEAR_REVERSE), log);
    }
}

void CarSensorData::on_radioButton_P_toggled() {
    if (mUi->radioButton_P->isChecked()) {
        string log("P");
        sendGearChangeMsg(static_cast<int32_t>(VehicleGear::GEAR_PARK), log);
    }
}

void CarSensorData::on_radioButton_N_toggled() {
    if (mUi->radioButton_N->isChecked()) {
        string log("N");
        sendGearChangeMsg(static_cast<int32_t>(VehicleGear::GEAR_NEUTRAL), log);
    }
}

void CarSensorData::on_radioButton_D1_toggled() {
    if (mUi->radioButton_D1->isChecked()) {
        string log("D1");
        sendGearChangeMsg(static_cast<int32_t>(VehicleGear::GEAR_1), log);
    }
}

void CarSensorData::on_radioButton_D2_toggled() {
    if (mUi->radioButton_D2->isChecked()) {
        string log("D2");
        sendGearChangeMsg(static_cast<int32_t>(VehicleGear::GEAR_2), log);
    }
}

void CarSensorData::on_radioButton_D3_toggled() {
    if (mUi->radioButton_D3->isChecked()) {
        string log("D3");
        sendGearChangeMsg(static_cast<int32_t>(VehicleGear::GEAR_3), log);
    }
}

void CarSensorData::on_radioButton_D4_toggled() {
    if (mUi->radioButton_D4->isChecked()) {
        string log("D4");
        sendGearChangeMsg(static_cast<int32_t>(VehicleGear::GEAR_4), log);
    }
}

void CarSensorData::on_radioButton_D5_toggled() {
    if (mUi->radioButton_D5->isChecked()) {
        string log("D5");
        sendGearChangeMsg(static_cast<int32_t>(VehicleGear::GEAR_5), log);
    }
}

void CarSensorData::on_radioButton_D6_toggled() {
    if (mUi->radioButton_D6->isChecked()) {
        string log("D6");
        sendGearChangeMsg(static_cast<int32_t>(VehicleGear::GEAR_6), log);
    }
}

void CarSensorData::on_radioButton_D7_toggled() {
    if (mUi->radioButton_D7->isChecked()) {
        string log("D7");
        sendGearChangeMsg(static_cast<int32_t>(VehicleGear::GEAR_7), log);
    }
}

void CarSensorData::on_radioButton_D8_toggled() {
    if (mUi->radioButton_D8->isChecked()) {
        string log("D8");
        sendGearChangeMsg(static_cast<int32_t>(VehicleGear::GEAR_8), log);
    }
}

void CarSensorData::on_radioButton_D9_toggled() {
    if (mUi->radioButton_D9->isChecked()) {
        string log("D9");
        sendGearChangeMsg(static_cast<int32_t>(VehicleGear::GEAR_9), log);
    }
}

void CarSensorData::on_checkBox_night_toggled() {
    bool night = mUi->checkBox_night->isChecked();
    EmulatorMessage emulatorMsg = makeSetPropMsg();
    VehiclePropValue* value = emulatorMsg.add_value();
    value->set_prop(static_cast<int32_t>(VehicleProperty::NIGHT_MODE));
    value->add_int32_values(night ? 1 : 0);
    string log = "Night mode: " + std::to_string(night);
    mSendEmulatorMsg(emulatorMsg, log);
}

void CarSensorData::on_radioButton_lock_toggled() {
    if (mUi->radioButton_lock->isChecked()) {
        string log("LOCK");
        sendIgnitionChangeMsg(static_cast<int32_t>(VehicleIgnitionState::LOCK),
                              log);
    }
}

void CarSensorData::on_radioButton_off_toggled() {
    if (mUi->radioButton_off->isChecked()) {
        string log("OFF");
        sendIgnitionChangeMsg(static_cast<int32_t>(VehicleIgnitionState::OFF),
                              log);
    }
}

void CarSensorData::on_radioButton_start_toggled() {
    if (mUi->radioButton_start->isChecked()) {
        string log("START");
        sendIgnitionChangeMsg(static_cast<int32_t>(VehicleIgnitionState::START),
                              log);
    }
}

void CarSensorData::on_radioButton_acc_toggled() {
    if (mUi->radioButton_acc->isChecked()) {
        string log("ACC");
        sendIgnitionChangeMsg(static_cast<int32_t>(VehicleIgnitionState::ACC),
                              log);
    }
}

void CarSensorData::on_radioButton_on_toggled() {
    if (mUi->radioButton_on->isChecked()) {
        string log("ON");
        sendIgnitionChangeMsg(static_cast<int32_t>(VehicleIgnitionState::ON),
                              log);
    }
}

void CarSensorData::on_checkBox_park_toggled() {
    bool parkBrakeOn = mUi->checkBox_park->isChecked();
    EmulatorMessage emulatorMsg = makeSetPropMsg();
    VehiclePropValue* value = emulatorMsg.add_value();
    value->set_prop(static_cast<int32_t>(VehicleProperty::PARKING_BRAKE_ON));
    value->add_int32_values(parkBrakeOn ? 1 : 0);
    string log = "Park brake: " + std::to_string(parkBrakeOn);
    mSendEmulatorMsg(emulatorMsg, log);
}

void CarSensorData::on_checkBox_unrestricted_toggled() {
    // unrestricted status and restricted status are mutually exclusive.
    if (mUi->checkBox_unrestricted->isChecked()) {
        mUi->checkBox_no_video->setChecked(false);
        mUi->checkBox_no_keyboard->setChecked(false);
        mUi->checkBox_limit_msg_len->setChecked(false);
        mUi->checkBox_no_config->setChecked(false);
        mUi->checkBox_no_voice->setChecked(false);
        string log("UNRESTRICTED");
        sendDrivingStatusChangeMsg(
                static_cast<int32_t>(VehicleDrivingStatus::UNRESTRICTED),
                log);
    }
}

void CarSensorData::on_checkBox_no_video_toggled() {
    collectDrivingStatusAndReport();
}

void CarSensorData::on_checkBox_no_keyboard_toggled() {
    collectDrivingStatusAndReport();
}

void CarSensorData::on_checkBox_limit_msg_len_toggled() {
    collectDrivingStatusAndReport();
}

void CarSensorData::on_checkBox_no_config_toggled() {
    collectDrivingStatusAndReport();
}

void CarSensorData::on_checkBox_no_voice_toggled() {
    collectDrivingStatusAndReport();
}

void CarSensorData::on_checkBox_fuel_low_toggled() {
    bool fuelLow = mUi->checkBox_fuel_low->isChecked();
    EmulatorMessage emulatorMsg = makeSetPropMsg();
    VehiclePropValue* value = emulatorMsg.add_value();
    value->set_prop(static_cast<int32_t>(VehicleProperty::FUEL_LEVEL_LOW));
    value->add_int32_values(fuelLow ? 1 : 0);
    string log = "Fuel low: " + std::to_string(fuelLow);
    mSendEmulatorMsg(emulatorMsg, log);
}
