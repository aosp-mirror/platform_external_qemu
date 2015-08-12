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

#include <QtWidgets>

#include "extended-window.h"
#include "ui_extended.h"
#include "android/sensors-agent.h"
#include "android/skin/qt/emulator-qt-window.h"

/*
 ********************************************************************************

  Acceleration (3 axis, default: 0:G:0.813417) (G=9.77622)
  Magnetic field (3 axis, default 0:0:0
  Orientation (what's this) (3 axis, default 0:0:0) (Az:Pitch:Roll)
  Temperature (Celsius) (prints 3 values? 0:x:x)
  Proximity (1:x:x)

  See console.c
      calls android_sensors_set( ID, val1, val2, val3 );
  in hw-sensors.c

  It appears that _set() and _get() just interact with a data
  structure in hw-sensors.c.
  hw-sensors.c autonomously interacts with the AVD

 */

void ExtendedWindow::initSensors()
{ }

void ExtendedWindow::on_sens_chargeCkBox_toggled(bool checked)
{
    sensorsState.isCharging = checked;
    if (sensorsAgent && sensorsAgent->setIsCharging) {
        sensorsAgent->setIsCharging(checked);
    }
}

void ExtendedWindow::on_sens_levelSlider_valueChanged(int value)
{
    sensorsState.chargeLevel = value;
    if (sensorsAgent && sensorsAgent->setChargeLevel) {
        sensorsAgent->setChargeLevel(value);
    }
}

void ExtendedWindow::on_sens_healthBox_currentIndexChanged(int index)
{
    SensorsHealth bHealth = (SensorsHealth)index;
    if (bHealth >= 0 && bHealth < Sensors_H_NUM_ENTRIES) {
        sensorsState.health = bHealth;

        if (sensorsAgent && sensorsAgent->setHealth) {
            sensorsAgent->setHealth(bHealth);
        }
    }
}

void ExtendedWindow::on_sens_statusBox_currentIndexChanged(int index)
{
    SensorsStatus bStatus = (SensorsStatus)index;
    if (bStatus >= 0 && bStatus < Sensors_S_NUM_ENTRIES) {
        sensorsState.status = bStatus;

        if (sensorsAgent && sensorsAgent->setStatus) {
                sensorsAgent->setStatus(bStatus);
        }
    }
}
