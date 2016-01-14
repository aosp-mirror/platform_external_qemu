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
#include "android/skin/qt/extended-pages/virtual-sensors-page.h"

#include "android/emulation/control/sensors_agent.h"
#include "android/hw-sensors.h"
#include <QQuaternion>

VirtualSensorsPage::VirtualSensorsPage(QWidget *parent) :
    QWidget(parent),
    mUi(new Ui::VirtualSensorsPage()),
    mSensorsAgent(nullptr)
{
    mUi->setupUi(this);
    mUi->yawAngleWidget->setRange(-180., 180);
    mUi->pitchAngleWidget->setRange(-180, 180);
    mUi->rollAngleWidget->setRange(-180, 180);
    mUi->temperatureSensorValueWidget->setRange(-273.1, 100.0);
    mUi->temperatureSensorValueWidget->setValue(25.0);
    mUi->lightSensorValueWidget->setRange(0, 40000.0);
    mUi->lightSensorValueWidget->setValue(20000.0);
    mUi->pressureSensorValueWidget->setRange(300, 1100);
    mUi->pressureSensorValueWidget->setValue(1013.25);
    mUi->humiditySensorValueWidget->setRange(0, 100);
    mUi->humiditySensorValueWidget->setValue(50);
    mUi->proximitySensorValueWidget->setRange(0, 10);
    mUi->proximitySensorValueWidget->setValue(10);
    mUi->magNorthWidget->setValidator(&mMagFieldValidator);
    mUi->magNorthWidget->setTextMargins(0, 0, 0, 4);
    mUi->magEastWidget->setValidator(&mMagFieldValidator);
    mUi->magEastWidget->setTextMargins(0, 0, 0, 4);
    mUi->magVerticalWidget->setValidator(&mMagFieldValidator);
    mUi->magVerticalWidget->setTextMargins(0, 0, 0, 4);

    onPhoneRotationChanged();

    connect(mUi->yawAngleWidget, SIGNAL(valueChanged()), this, SLOT(onPhoneRotationChanged()));
    connect(mUi->pitchAngleWidget, SIGNAL(valueChanged()), this, SLOT(onPhoneRotationChanged()));
    connect(mUi->rollAngleWidget, SIGNAL(valueChanged()), this, SLOT(onPhoneRotationChanged()));
    connect(mUi->magNorthWidget, SIGNAL(editingFinished()), this, SLOT(onPhoneRotationChanged()));
    connect(mUi->magEastWidget, SIGNAL(editingFinished()), this, SLOT(onPhoneRotationChanged()));
    connect(mUi->magVerticalWidget, SIGNAL(editingFinished()), this, SLOT(onPhoneRotationChanged()));
}

void VirtualSensorsPage::setSensorsAgent(const QAndroidSensorsAgent* agent) {
    mSensorsAgent = agent;

    // Update the agent with current values.
    onPhoneRotationChanged();
}

// Helper function
static void setSensorValue(
        const QAndroidSensorsAgent* agent,
        AndroidSensor sensor_id,
        double v1,
        double v2 = 0.0,
        double v3 = 0.0) {
    if (agent) {
        agent->setSensor(sensor_id, 
                         static_cast<float>(v1),
                         static_cast<float>(v2),
                         static_cast<float>(v3));
    }
}

void VirtualSensorsPage::on_temperatureSensorValueWidget_valueChanged(double value) {
    setSensorValue(mSensorsAgent, ANDROID_SENSOR_TEMPERATURE, value);
}

void VirtualSensorsPage::on_proximitySensorValueWidget_valueChanged(double value) {
    setSensorValue(mSensorsAgent, ANDROID_SENSOR_PROXIMITY, value);
}

void VirtualSensorsPage::on_lightSensorValueWidget_valueChanged(double value) {
    setSensorValue(mSensorsAgent, ANDROID_SENSOR_LIGHT, value);
}

void VirtualSensorsPage::on_pressureSensorValueWidget_valueChanged(double value) {
    setSensorValue(mSensorsAgent, ANDROID_SENSOR_PRESSURE, value);
}

void VirtualSensorsPage::on_humiditySensorValueWidget_valueChanged(double value) {
    setSensorValue(mSensorsAgent, ANDROID_SENSOR_HUMIDITY, value);
}

// Helper function.
static QString formatSensorValue(double value) {
    return QString("%1").arg(value, 8, 'f', 2, ' ');
}

void VirtualSensorsPage::onPhoneRotationChanged() {
    // Yaw is rotation around Z axis.
    // Pitch is rotation around X axis.
    // When the device is positioned on a table with screen
    // up, the Z axis points from the screen towards the sky,
    // the X axis points to the top of the device.
    // The "top" can mean the short or the long edge, depending
    // on whether the device is in portrait mode or in landscape
    // mode.
    double yaw = mUi->yawAngleWidget->getValue();
    double pitch = mUi->pitchAngleWidget->getValue();
    double roll = mUi->rollAngleWidget->getValue();

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
            mUi->magNorthWidget->text().toDouble(),
            mUi->magEastWidget->text().toDouble(),
            mUi->magVerticalWidget->text().toDouble());

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
    setSensorValue(mSensorsAgent,
                   ANDROID_SENSOR_ACCELERATION,
                   device_gravity_vector.x(),
                   device_gravity_vector.y(),
                   device_gravity_vector.z());

    setSensorValue(mSensorsAgent,
                   ANDROID_SENSOR_MAGNETIC_FIELD,
                   device_magnetic_vector.x(),
                   device_magnetic_vector.y(),
                   device_magnetic_vector.z());
    
    setSensorValue(mSensorsAgent,
                   ANDROID_SENSOR_ORIENTATION,
                   yaw,
                   pitch,
                   roll);

    // Update labels with new values.
    mUi->accelerometerXLabel->setText(formatSensorValue(device_gravity_vector.x()));
    mUi->accelerometerYLabel->setText(formatSensorValue(device_gravity_vector.y()));
    mUi->accelerometerZLabel->setText(formatSensorValue(device_gravity_vector.z()));
    mUi->magnetometerNorthLabel->setText(formatSensorValue(device_magnetic_vector.x()));
    mUi->magnetometerEastLabel->setText(formatSensorValue(device_magnetic_vector.y()));
    mUi->magnetometerVerticalLabel->setText(formatSensorValue(device_magnetic_vector.z()));
}

