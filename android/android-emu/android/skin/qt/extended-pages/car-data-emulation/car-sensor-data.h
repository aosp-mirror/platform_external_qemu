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

#define MPH_TO_MS 0.44704
#define KMS_To_MS 0.27778
#define ERROR_SPEED -1
#define MILE_PER_HOUR 0
#define KM_PER_HOUR 1
#define M_PER_SEC 2

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
    void sendGearChangeMsg(const int gear, const std::string& gearName);
    void sendIgnitionChangeMsg(const int ignition,
                               const std::string& ignitionName);
    float getStandardSpeed(int speed, int unitIndex);
};
