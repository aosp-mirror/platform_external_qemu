/* Copyright (C) 2015 The Android Open Source Project
 **
 ** This software is licensed under the terms of the GNU General Public
 ** License version 2, as published by the Free Software Foundation, and
 ** may be copied, distributed, and modified under those terms.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 */

#include "android/skin/qt/extended-window.h"
#include "ui_extended.h"
#include <QQuaternion>
#include <iostream>

void ExtendedWindow::initVirtualSensors()
{
    mExtendedUi->yawAngleWidget->setRange(-360., 360);
    mExtendedUi->pitchAngleWidget->setRange(-360, 360);
    mExtendedUi->rollAngleWidget->setRange(-360, 360);
    mExtendedUi->temperatureSensorValueWidget->setRange(-273.1, 100.0);
    mExtendedUi->proximitySensorValueWidget->setRange(0, 10);
    onPhoneRotationChanged();

    connect(mExtendedUi->yawAngleWidget, SIGNAL(valueChanged()), this, SLOT(onPhoneRotationChanged()));
    connect(mExtendedUi->pitchAngleWidget, SIGNAL(valueChanged()), this, SLOT(onPhoneRotationChanged()));
    connect(mExtendedUi->rollAngleWidget, SIGNAL(valueChanged()), this, SLOT(onPhoneRotationChanged()));
}

void ExtendedWindow::on_temperatureSensorValueWidget_valueChanged(double value) {
    mSensorsAgent->setSensor(ANDROID_SENSOR_TEMPERATURE, static_cast<float>(value), .0f, .0f);
}

void ExtendedWindow::on_proximitySensorValueWidget_valueChanged(double value) {
    mSensorsAgent->setSensor(ANDROID_SENSOR_PROXIMITY, static_cast<float>(value), .0f, .0f);
}

void ExtendedWindow::onPhoneRotationChanged() {
    std::cout << "Rotation changed!\n"; 
    double yaw = mExtendedUi->yawAngleWidget->getValue();
    double pitch = mExtendedUi->pitchAngleWidget->getValue();
    double roll = mExtendedUi->rollAngleWidget->getValue();

    QQuaternion phone_rotation_quat =
        QQuaternion::fromAxisAndAngle(0, 0, 1.0, yaw) *
        QQuaternion::fromAxisAndAngle(1.0, 0, 0, pitch) *
        QQuaternion::fromAxisAndAngle(0, 1.0, 0, roll);
    QVector3D gravity_vector(0, 0, -9.8);
    QVector3D acceleration_vector = phone_rotation_quat.conjugate().rotatedVector(gravity_vector);
    mSensorsAgent->setSensor(ANDROID_SENSOR_ACCELERATION,
                             static_cast<float>(acceleration_vector.x()),
                             static_cast<float>(acceleration_vector.y()),
                             static_cast<float>(acceleration_vector.z()));
    mExtendedUi->accelerometerValuesLabel->setText(
            "(" + QString::number(acceleration_vector.x()) + ", " +
            QString::number(acceleration_vector.y()) + ", " +
            QString::number(acceleration_vector.z()) + ")");
}

