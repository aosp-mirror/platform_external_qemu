// Copyright (C) 2020 The Android Open Source Project
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

#include <qobjectdefs.h>       // for Q_OBJECT
#include <QString>           // for QString
#include <QWidget>                                          // for QWidget
#include <QLabel>
#include <memory>                                           // for unique_ptr


#include "ui_car-sensor-replay-page.h"  // for CarSensorReplayPage

class QObject;
class QWidget;
class QLabel;
class QObject;

class CarSensorReplayPage : public QWidget {
    Q_OBJECT

public:
    explicit CarSensorReplayPage(QWidget* parent = nullptr);

private slots:
    void on_sensor_LoadSensorButton_clicked();
    void on_sensor_playStopButton_clicked();


private:

    std::unique_ptr<Ui::CarSensorReplayPage> mUi;
};
