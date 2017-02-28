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

#include <QWidget>
#include <functional>
#include <memory>
#include "android/emulation/proto/VehicleHalProto.pb.h"
#include "ui_car-sensor-data.h"

using std::function;
using std::string;
using emulator::EmulatorMessage;

struct QCarDataAgent;
class CarSensorData : public QWidget {
    Q_OBJECT
public:
    explicit CarSensorData(QWidget* parent = nullptr);
    void setSendEmulatorMsg(
            const function<void(const EmulatorMessage msg, const string log)>&);

private slots:
    void on_car_speedSlider_valueChanged(int value);
    void on_radioButton_D_toggled();
    void on_radioButton_R_toggled();
    void on_radioButton_P_toggled();
    void on_radioButton_N_toggled();
    void on_radioButton_D1_toggled();
    void on_radioButton_D2_toggled();
    void on_radioButton_D3_toggled();
    void on_radioButton_D4_toggled();
    void on_radioButton_D5_toggled();
    void on_radioButton_D6_toggled();
    void on_radioButton_D7_toggled();
    void on_radioButton_D8_toggled();
    void on_radioButton_D9_toggled();

    void on_radioButton_lock_toggled();
    void on_radioButton_off_toggled();
    void on_radioButton_start_toggled();
    void on_radioButton_acc_toggled();
    void on_radioButton_on_toggled();

    void on_checkBox_night_toggled();
    void on_checkBox_park_toggled();

    void on_checkBox_unrestricted_toggled();
    void on_checkBox_no_video_toggled();
    void on_checkBox_no_keyboard_toggled();
    void on_checkBox_limit_msg_len_toggled();
    void on_checkBox_no_config_toggled();
    void on_checkBox_no_voice_toggled();

    void on_checkBox_fuel_low_toggled();

private:
    std::unique_ptr<Ui::CarSensorData> mUi;
    function<void(const EmulatorMessage msg, const string log)>
            sendEmulatorMsg = nullptr;
    void sendGearChangeMsg(int gear, string gearName);
    void sendDrivingStatusChangeMsg(int status, string statusName);
    void sendIgnitionChangeMsg(int ignition, string ignitionName);
    void collectDrivingStatusAndReport();
};