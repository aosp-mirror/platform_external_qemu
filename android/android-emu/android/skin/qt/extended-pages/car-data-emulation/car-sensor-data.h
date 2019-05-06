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

#include "ui_car-sensor-data.h"

#include <QWidget>
#include <functional>
#include <memory>

struct QCarDataAgent;
namespace emulator {
class EmulatorMessage;
}
class CarSensorData : public QWidget {
    Q_OBJECT
public:
    explicit CarSensorData(QWidget* parent = nullptr);
    using EmulatorMsgCallback =
            std::function<void(const emulator::EmulatorMessage& msg,
                               const std::string& log)>;
    void setSendEmulatorMsgCallback(EmulatorMsgCallback&&);
    void processMsg(emulator::EmulatorMessage emulatorMsg);

private slots:
    void on_car_speedSlider_valueChanged(int value);
    void on_comboBox_gear_currentIndexChanged(int index);
    void on_comboBox_ignition_currentIndexChanged(int index);
    void on_checkBox_night_toggled();
    void on_checkBox_park_toggled();
    void on_checkBox_fuel_low_toggled();

private:
    std::unique_ptr<Ui::CarSensorData> mUi;
    EmulatorMsgCallback mSendEmulatorMsg;
    const float MILES_PER_HOUR_TO_METERS_PER_SEC = 1.60934f*1000.0f/(60.0f*60.0f);
    const float KILOMETERS_PER_HOUR_TO_METERS_PER_SEC = 1000.0f/(60.0f*60.0f);
    enum SpeedUnitSelection {
        MILES_PER_HOUR = 0,
        KILOMETERS_PER_HOUR = 1
    };
    void sendGearChangeMsg(const int gear, const std::string& gearName);
    void sendIgnitionChangeMsg(const int ignition,
                               const std::string& ignitionName);
    float getSpeedMetersPerSecond(int speed, int unitIndex);
    int getIndexFromVehicleGear(int gear);
};
