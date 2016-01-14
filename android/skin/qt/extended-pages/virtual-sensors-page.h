// Copyright (C) 2015 The Android Open Source Project
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

#include "ui_virtual-sensors-page.h"
#include <QDoubleValidator>
#include <QWidget>
#include <memory>

struct QAndroidSensorsAgent;
class VirtualSensorsPage : public QWidget
{
    Q_OBJECT

public:
    explicit VirtualSensorsPage(QWidget *parent = 0);

    void setSensorsAgent(const QAndroidSensorsAgent* agent);

private slots:
    void on_temperatureSensorValueWidget_valueChanged(double value);
    void on_proximitySensorValueWidget_valueChanged(double value);
    void on_lightSensorValueWidget_valueChanged(double value);
    void on_pressureSensorValueWidget_valueChanged(double value);
    void on_humiditySensorValueWidget_valueChanged(double value);
    void onPhoneRotationChanged();

private:
    std::unique_ptr<Ui::VirtualSensorsPage> mUi;
    QDoubleValidator mMagFieldValidator;
    const QAndroidSensorsAgent* mSensorsAgent;
};

