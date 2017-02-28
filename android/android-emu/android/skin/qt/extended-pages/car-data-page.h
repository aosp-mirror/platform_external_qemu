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

#include "ui_car-data-page.h"

#include <QWidget>
#include <memory>
#include "android/emulation/proto/VehicleHalProto.pb.h"

using emulator::EmulatorMessage;

struct QCarDataAgent;
class CarDataPage : public QWidget {
    Q_OBJECT
public:
    explicit CarDataPage(QWidget* parent = nullptr);
    static void carDataCallback(const char* msg, int length, void* context);
    void setCarDataAgent(const QCarDataAgent* agent);
    void onReceiveData(const char* msg, int length);
    void sendCarEmulatorMessageLogged(EmulatorMessage msg, string log);

signals:
    void requestUpdateData(QString msg);

private slots:
    void on_car_sendDataButton_clicked();

private:
    std::unique_ptr<Ui::CarDataPage> mUi;
    const QCarDataAgent* mCarDataAgent = nullptr;
    void updateReceivedData(const QString msg);
};