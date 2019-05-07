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

#include "android/emulation/proto/VehicleHalProto.pb.h"
#include "android/skin/qt/qt-settings.h"
#include "android/utils/debug.h"
#include "ui_car-sensor-data.h"
#include "vehicle_constants_generated.h"

#include <QSettings>

#define D(...) VERBOSE_PRINT(car, __VA_ARGS__)

using std::string;

using emulator::EmulatorMessage;
using emulator::MsgType;
using emulator::Status;
using emulator::VehicleProperty;
using emulator::VehiclePropValue;
using emulator::VehicleGear;
using emulator::VehicleIgnitionState;

CarSensorData::CarSensorData(QWidget* parent)
    : QWidget(parent), mUi(new Ui::CarSensorData) {
    mUi->setupUi(this);
}

static const enum VehicleGear sComboBoxGearValues[] = {
        VehicleGear::GEAR_NEUTRAL, VehicleGear::GEAR_REVERSE,
        VehicleGear::GEAR_PARK,    VehicleGear::GEAR_DRIVE};

static const enum VehicleIgnitionState sComboBoxIgnitionStates[] = {
        VehicleIgnitionState::UNDEFINED, VehicleIgnitionState::LOCK,
        VehicleIgnitionState::OFF,       VehicleIgnitionState::ACC,
        VehicleIgnitionState::ON,        VehicleIgnitionState::START};

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
    value->set_prop(static_cast<int32_t>(VehicleProperty::GEAR_SELECTION));
    value->add_int32_values(gear);
    string log = "Gear changed to " + gearName;
    mSendEmulatorMsg(emulatorMsg, log);
}

void CarSensorData::sendIgnitionChangeMsg(const int ignition,
                                          const string& ignitionName) {
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
    mUi->car_speedLabel->setText(QString::number(speed));
    float speedMetersPerSecond = (float)speed * 
        ((mUi->comboBox_speedUnit->currentIndex() == MILES_PER_HOUR) 
            ? MILES_PER_HOUR_TO_METERS_PER_SEC
            : KILOMETERS_PER_HOUR_TO_METERS_PER_SEC);
    
    if (mSendEmulatorMsg != nullptr) {
        EmulatorMessage emulatorMsg = makeSetPropMsg();
        VehiclePropValue* value = emulatorMsg.add_value();
        value->set_prop(
                static_cast<int32_t>(VehicleProperty::PERF_VEHICLE_SPEED));
        value->add_float_values(speedMetersPerSecond);
        string log = "Speed changed to " + std::to_string(speedMetersPerSecond);
        mSendEmulatorMsg(emulatorMsg, log);
    }
}

void CarSensorData::setSendEmulatorMsgCallback(EmulatorMsgCallback&& func) {
    mSendEmulatorMsg = std::move(func);
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

void CarSensorData::on_checkBox_park_toggled() {
    bool parkBrakeOn = mUi->checkBox_park->isChecked();
    EmulatorMessage emulatorMsg = makeSetPropMsg();
    VehiclePropValue* value = emulatorMsg.add_value();
    value->set_prop(static_cast<int32_t>(VehicleProperty::PARKING_BRAKE_ON));
    value->add_int32_values(parkBrakeOn ? 1 : 0);
    string log = "Park brake: " + std::to_string(parkBrakeOn);
    mSendEmulatorMsg(emulatorMsg, log);
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

void CarSensorData::on_comboBox_ignition_currentIndexChanged(int index) {
    sendIgnitionChangeMsg(static_cast<int32_t>(sComboBoxIgnitionStates[index]),
                          mUi->comboBox_ignition->currentText().toStdString());
}

void CarSensorData::on_comboBox_gear_currentIndexChanged(int index) {
    sendGearChangeMsg(static_cast<int32_t>(sComboBoxGearValues[index]),
                      mUi->comboBox_gear->currentText().toStdString());
}

void CarSensorData::processMsg(emulator::EmulatorMessage emulatorMsg) {
    if (emulatorMsg.prop_size() == 0 && emulatorMsg.value_size() == 0) {
        return;
    }
    for (int valIndex = 0; valIndex < emulatorMsg.value_size(); valIndex++) {
        VehiclePropValue val = emulatorMsg.value(valIndex);

        switch (val.prop()) {
            case static_cast<int32_t>(VehicleProperty::GEAR_SELECTION): {
                int gear_vhal = getIndexFromVehicleGear(val.int32_values(0));
                int gear_state = mUi->comboBox_gear->currentIndex();
                if (gear_state != gear_vhal) {
                    mUi->comboBox_gear->setCurrentIndex(gear_vhal);
                }
                break;
            }
            case static_cast<int32_t>(VehicleProperty::FUEL_LEVEL_LOW): {
                bool fuelLow_vhal = val.int32_values(0) == 1;
                bool fuelLow = mUi->checkBox_fuel_low->isChecked();
                if (fuelLow != fuelLow_vhal) {
                    mUi->checkBox_fuel_low->setChecked(fuelLow_vhal);
                }
                break;
            }
            case static_cast<int32_t>(VehicleProperty::PARKING_BRAKE_ON): {
                bool parkingBreak_vhal = val.int32_values(0) == 1;
                bool parkingBreak = mUi->checkBox_park->isChecked();
                if (parkingBreak != parkingBreak_vhal) {
                    mUi->checkBox_park->setChecked(parkingBreak_vhal);
                }
                break;
            }
            case static_cast<int32_t>(VehicleProperty::NIGHT_MODE): {
                bool night_vhal = val.int32_values(0) == 1;
                bool night = mUi->checkBox_night->isChecked();
                if (night != night_vhal) {
                    mUi->checkBox_night->setChecked(night_vhal);
                }
                break;
            }
            case static_cast<int32_t>(VehicleProperty::PERF_VEHICLE_SPEED): {
                float speedMeterPerSecond = val.float_values(0);
                int speed = static_cast<int32_t>(
                        speedMeterPerSecond /
                        ((mUi->comboBox_speedUnit->currentIndex() ==
                          MILES_PER_HOUR)
                                 ? MILES_PER_HOUR_TO_METERS_PER_SEC
                                 : KILOMETERS_PER_HOUR_TO_METERS_PER_SEC));
                if (speed != mUi->car_speedSlider->value()) {
                    mUi->car_speedSlider->setValue(speed);
                }
                break;
            }
            case static_cast<int32_t>(VehicleProperty::IGNITION_STATE): {
                int ignition_state_vhal = val.int32_values(0);
                int ignition_state = mUi->comboBox_ignition->currentIndex();
                if (ignition_state != ignition_state_vhal) {
                    mUi->comboBox_ignition->setCurrentIndex(
                            ignition_state_vhal);
                }
                break;
            }
            default: {
                break;
            }
        }
    }
}

int CarSensorData::getIndexFromVehicleGear(int gear) {
    int len = sizeof(sComboBoxGearValues) / sizeof(sComboBoxGearValues[0]);
    for (int index = 0; index < len; index++) {
        if (static_cast<int>(sComboBoxGearValues[index]) == gear) {
            return index;
        }
    }
    return len - 1;
}
