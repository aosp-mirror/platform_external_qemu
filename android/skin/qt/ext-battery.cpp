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

#include "android/emulation/control/battery_agent.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "extended-window.h"
#include "ui_extended.h"

void ExtendedWindow::initBattery()
{ }

void ExtendedWindow::on_bat_chargerBox_currentIndexChanged(int index)
{
    BatteryCharger bCharger = (BatteryCharger)index;
    if (bCharger >= 0 && bCharger < BATTERY_CHARGER_NUM_ENTRIES) {
        mBatteryState.mCharger = bCharger;

        if (mBatteryAgent && mBatteryAgent->setCharger) {
            mBatteryAgent->setCharger(bCharger);
        }
    }
}

void ExtendedWindow::on_bat_levelSlider_valueChanged(int value)
{
    mBatteryState.mChargeLevel = value;
    // Update the text output
    mExtendedUi->bat_chargeLevelText->setText(QString::number(value)+"%");

    if (mBatteryAgent && mBatteryAgent->setChargeLevel) {
        mBatteryAgent->setChargeLevel(value);
    }
}

void ExtendedWindow::on_bat_healthBox_currentIndexChanged(int index)
{
    BatteryHealth bHealth = (BatteryHealth)index;
    if (bHealth >= 0 && bHealth < BATTERY_HEALTH_NUM_ENTRIES) {
        mBatteryState.mHealth = bHealth;

        if (mBatteryAgent && mBatteryAgent->setHealth) {
            mBatteryAgent->setHealth(bHealth);
        }
    }
}

void ExtendedWindow::on_bat_statusBox_currentIndexChanged(int index)
{
    BatteryStatus bStatus = (BatteryStatus)index;
    if (bStatus >= 0 && bStatus < BATTERY_STATUS_NUM_ENTRIES) {
        mBatteryState.mStatus = bStatus;

        if (mBatteryAgent && mBatteryAgent->setStatus) {
                mBatteryAgent->setStatus(bStatus);
        }
    }
}
