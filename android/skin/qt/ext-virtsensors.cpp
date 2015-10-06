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
#include "android/hw-sensors.h"
#include "ui_extended.h"
#include <limits>
#include <iostream>

void ExtendedWindow::initVirtualSensors()
{
    // This will allow to set any valid double as a
    // value returned from sensor.
    mExtendedUi->virtsensors_valueASpinBox->setRange(
            -std::numeric_limits<double>::max(),
            std::numeric_limits<double>::max());
    mExtendedUi->virtsensors_valueBSpinBox->setRange(
            -std::numeric_limits<double>::max(),
            std::numeric_limits<double>::max());
    mExtendedUi->virtsensors_valueCSpinBox->setRange(
            -std::numeric_limits<double>::max(),
            std::numeric_limits<double>::max());

    // Go through each sensor type and fill out sensor-specific
    // information, such as units of measurement of each value.
    for (int sensor_id = 0; sensor_id < MAX_SENSORS; ++sensor_id) {
        VirtualSensorInfo &sensor_info = mVirtualSensors[sensor_id];
        switch(sensor_id) {
        case ANDROID_SENSOR_ACCELERATION:
            sensor_info.name = "Acceleration";
            sensor_info.value_infos.push_back({"X", "m/s^2"});
            sensor_info.value_infos.push_back({"Y", "m/s^2"});
            sensor_info.value_infos.push_back({"Z", "m/s^2"});
            break;
        case ANDROID_SENSOR_MAGNETIC_FIELD:
            sensor_info.name = "Magnetic Field";
            sensor_info.value_infos.push_back({"X", "microtesla"});
            sensor_info.value_infos.push_back({"Y", "microtesla"});
            sensor_info.value_infos.push_back({"Z", "microtesla"});
            break;
        case ANDROID_SENSOR_ORIENTATION:
            sensor_info.name = "Orientation";
            sensor_info.value_infos.push_back({"X", "degrees"});
            sensor_info.value_infos.push_back({"Y", "degrees"});
            sensor_info.value_infos.push_back({"Z", "degrees"});
            break;
        case ANDROID_SENSOR_TEMPERATURE:
            sensor_info.name = "Ambient Temperature";
            sensor_info.value_infos.push_back({"", "degrees Celsius"});
            break;
        case ANDROID_SENSOR_PROXIMITY:
            sensor_info.name = "Proximity";
            sensor_info.value_infos.push_back({"", "cm"});
            break;
        }
        mExtendedUi->virtsensors_sensorType->addItem(sensor_info.name, QVariant(sensor_id));
    }

    // Adjust the UI for the sensor type that is selected by default.
    adjustUiForVirtualSensor(0);
}

void ExtendedWindow::adjustUiForVirtualSensor(int sensor_id) {
    if (sensor_id >= 0 && sensor_id < MAX_SENSORS) { // ensure valid id
        VirtualSensorInfo &sensor_info = mVirtualSensors[sensor_id];
        QDoubleSpinBox *const value_spin_boxes[] = {
            mExtendedUi->virtsensors_valueASpinBox,
            mExtendedUi->virtsensors_valueBSpinBox,
            mExtendedUi->virtsensors_valueCSpinBox,
        };
        float values[3] = {.0f, .0f, .0f};
        mSelectedSensorId = sensor_id;
        if (mSensorsAgent) {
            mSensorsAgent->getSensor(sensor_id, &values[0], &values[1], &values[2]);
        }
        for (size_t v = 0; v < sizeof(value_spin_boxes)/sizeof(QDoubleSpinBox*); ++v) {
            QDoubleSpinBox *spin_box = value_spin_boxes[v];
            
            if (v >= sensor_info.value_infos.size()) {
                // The newly selected sensor type doesn't have this value, hide
                // the corresponding spin box.
                spin_box->hide();
            } else {
                // The newly selected sesor type has this value, make sure the
                // spinbox is visible, has the correct prefix and suffix, and
                // restore the previously remembered value into the spinbox.
                spin_box->show();
                spin_box->setPrefix(sensor_info.value_infos[v].name + ": ");
                spin_box->setSuffix(" " + sensor_info.value_infos[v].units);
                spin_box->setValue(values[v]);
            }
        }
    }
}

void ExtendedWindow::updateSensorValues()
{
    if (mSensorsAgent && mSelectedSensorId > -1) {
        mSensorsAgent->setSensor(
                mSelectedSensorId,
                mExtendedUi->virtsensors_valueASpinBox->value(),
                mExtendedUi->virtsensors_valueBSpinBox->value(),
                mExtendedUi->virtsensors_valueCSpinBox->value());
    }
}

void ExtendedWindow::on_virtsensors_sensorType_currentIndexChanged(int new_index)
{
    adjustUiForVirtualSensor(mExtendedUi->virtsensors_sensorType->itemData(new_index).toInt());
}

void ExtendedWindow::on_virtsensors_valueASpinBox_valueChanged(double)
{
    updateSensorValues();
}

void ExtendedWindow::on_virtsensors_valueBSpinBox_valueChanged(double)
{
    updateSensorValues();
}

void ExtendedWindow::on_virtsensors_valueCSpinBox_valueChanged(double)
{
    updateSensorValues();
}
