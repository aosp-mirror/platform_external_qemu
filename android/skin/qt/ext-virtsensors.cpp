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

void ExtendedWindow::initVirtualSensors()
{
    mExtendedUi->yawAngleWidget->setRange(-360., 360);
    mExtendedUi->pitchAngleWidget->setRange(-360, 360);
    mExtendedUi->rollAngleWidget->setRange(-360, 360);
    mExtendedUi->temperatureSensorValueWidget->setRange(-273.1, 100.0);
    mExtendedUi->proximitySensorValueWidget->setRange(0, 10);
    mExtendedUi->magNorthWidget->setValidator(&mMagFieldValidator);
    mExtendedUi->magNorthWidget->setTextMargins(0, 0, 0, 4);
    mExtendedUi->magEastWidget->setValidator(&mMagFieldValidator);
    mExtendedUi->magEastWidget->setTextMargins(0, 0, 0, 4);
    mExtendedUi->magVerticalWidget->setValidator(&mMagFieldValidator);
    mExtendedUi->magVerticalWidget->setTextMargins(0, 0, 0, 4);

    onPhoneRotationChanged();

    connect(mExtendedUi->yawAngleWidget, SIGNAL(valueChanged()), this, SLOT(onPhoneRotationChanged()));
    connect(mExtendedUi->pitchAngleWidget, SIGNAL(valueChanged()), this, SLOT(onPhoneRotationChanged()));
    connect(mExtendedUi->rollAngleWidget, SIGNAL(valueChanged()), this, SLOT(onPhoneRotationChanged()));
    connect(mExtendedUi->magNorthWidget, SIGNAL(editingFinished()), this, SLOT(onPhoneRotationChanged()));
    connect(mExtendedUi->magEastWidget, SIGNAL(editingFinished()), this, SLOT(onPhoneRotationChanged()));
    connect(mExtendedUi->magVerticalWidget, SIGNAL(editingFinished()), this, SLOT(onPhoneRotationChanged()));
}

void ExtendedWindow::on_temperatureSensorValueWidget_valueChanged(double value) {
    mSensorsAgent->setSensor(ANDROID_SENSOR_TEMPERATURE, static_cast<float>(value), .0f, .0f);
}

void ExtendedWindow::on_proximitySensorValueWidget_valueChanged(double value) {
    mSensorsAgent->setSensor(ANDROID_SENSOR_PROXIMITY, static_cast<float>(value), .0f, .0f);
}

void ExtendedWindow::onPhoneRotationChanged() {
    // Yaw is rotation around Z axis.
    // Pitch is rotation around X axis.
    // When the device is positioned on a table with screen
    // up, the Z axis points from the screen towards the sky,
    // the X axis points to the top of the device.
    // The "top" can mean the short or the long edge, depending
    // on whether the device is in portrait mode or in landscape
    // mode.
    double yaw = mExtendedUi->yawAngleWidget->getValue();
    double pitch = mExtendedUi->pitchAngleWidget->getValue();
    double roll = mExtendedUi->rollAngleWidget->getValue();

    // A quaternion representing the device's rotation.
    // Yaw, pitch and roll are applied in that order.
    QQuaternion device_rotation_quat =
        QQuaternion::fromAxisAndAngle(0, 0, 1.0, yaw) *
        QQuaternion::fromAxisAndAngle(1.0, 0, 0, pitch) *
        QQuaternion::fromAxisAndAngle(0, 1.0, 0, roll);

    // Gravity and magnetic vector in the "absolute" frame of
    // reference (i.e. device operator's frame of reference).
    QVector3D gravity_vector(0, 0, -9.8);
    QVector3D magnetic_vector(
            mExtendedUi->magNorthWidget->text().toDouble(),
            mExtendedUi->magEastWidget->text().toDouble(),
            mExtendedUi->magVerticalWidget->text().toDouble());

    // Gravity and magnetic vectors as observed by the device.
    // Note how we're applying the *inverse* of the transformation
    // represented by device_rotation_quat to the "absolute" coordinates
    // of the vectors.
    QVector3D device_gravity_vector =
        device_rotation_quat.conjugate().rotatedVector(gravity_vector);
    QVector3D device_magnetic_vector =
        device_rotation_quat.conjugate().rotatedVector(magnetic_vector);

    // Acceleration is affected both by the gravity and linear movement of the device.
    // For now, we don't have a linear component, so just account for gravity.
    mSensorsAgent->setSensor(ANDROID_SENSOR_ACCELERATION,
                             static_cast<float>(device_gravity_vector.x()),
                             static_cast<float>(device_gravity_vector.y()),
                             static_cast<float>(device_gravity_vector.z()));
    mSensorsAgent->setSensor(ANDROID_SENSOR_MAGNETIC_FIELD,
                             static_cast<float>(device_magnetic_vector.x()),
                             static_cast<float>(device_magnetic_vector.y()),
                             static_cast<float>(device_magnetic_vector.z()));
    mSensorsAgent->setSensor(ANDROID_SENSOR_ORIENTATION,
                             static_cast<float>(yaw),
                             static_cast<float>(pitch),
                             static_cast<float>(roll));

    // Update labels with new values.
    mExtendedUi->accelerometerValuesLabel->setText(
            "(" +
            QString::number(device_gravity_vector.x(), 'f', 3) + ", " +
            QString::number(device_gravity_vector.y(), 'f', 3) + ", " +
            QString::number(device_gravity_vector.z(), 'f', 3) + ")");
    mExtendedUi->magnetometerValuesLabel->setText(
            "(" +
            QString::number(device_magnetic_vector.x(), 'f', 3) + ", " +
            QString::number(device_magnetic_vector.y(), 'f', 3) + ", " +
            QString::number(device_magnetic_vector.z(), 'f', 3) + ")");
}

