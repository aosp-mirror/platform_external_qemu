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

struct charging_s {
    const BatteryCharger theEnum;
    const char           theString[32];
};

static const struct charging_s chargingAssociations[] = {
    { BATTERY_CHARGER_NONE,     "None"             },
    { BATTERY_CHARGER_AC,       "AC charger"       },
// TODO: The UI only allows NONE and AC.
//       Need to also allow and implement USB and WIRELESS.
//  { BATTERY_CHARGER_USB,      "USB charger"      },
//  { BATTERY_CHARGER_WIRELESS, "Wireless charger" }
};
#define NUM_CHARGINGS (sizeof(chargingAssociations)/sizeof(chargingAssociations[0]))

struct health_s {
    const BatteryHealth theEnum;
    const char          theString[32];
};

static const struct health_s healthAssociations[] = {
    { BATTERY_HEALTH_GOOD,        "Good"        },
    { BATTERY_HEALTH_FAILED,      "Failed"      },
    { BATTERY_HEALTH_DEAD,        "Dead"        },
    { BATTERY_HEALTH_OVERVOLTAGE, "Overvoltage" },
    { BATTERY_HEALTH_OVERHEATED,  "Overheated"  },
    { BATTERY_HEALTH_UNKNOWN,     "Unknown"     },
};
#define NUM_HEALTHS (sizeof(healthAssociations)/sizeof(healthAssociations[0]))

struct status_s {
    const BatteryStatus theEnum;
    const char          theString[32];
};

static const struct status_s statusAssociations[] = {
    { BATTERY_STATUS_UNKNOWN,      "Unknown"      },
    { BATTERY_STATUS_CHARGING,     "Charging"     },
    { BATTERY_STATUS_DISCHARGING,  "Discharging"  },
    { BATTERY_STATUS_NOT_CHARGING, "Not charging" },
    { BATTERY_STATUS_FULL,         "Full"         },
};
#define NUM_STATUSES (sizeof(statusAssociations)/sizeof(statusAssociations[0]))

////////////////////////////////////////////////////////////////////////////////

void ExtendedWindow::initBattery()
{
    //////////////////////
    // Charge level
    int chargeLevel = 50;
    if (mBatteryAgent &&  mBatteryAgent->chargeLevel) {
        chargeLevel = mBatteryAgent->chargeLevel();
    }
    mExtendedUi->bat_levelSlider->setValue(chargeLevel);

    //////////////////////
    // Charger connection
    mExtendedUi->bat_chargerBox->clear();
    for (unsigned int idx = 0; idx < NUM_CHARGINGS; idx++) {
        mExtendedUi->bat_chargerBox->addItem(
                tr(chargingAssociations[idx].theString),
                chargingAssociations[idx].theEnum );
    }

    BatteryCharger bCharger = BATTERY_CHARGER_AC;
    if (mBatteryAgent &&  mBatteryAgent->charger) {
        bCharger = mBatteryAgent->charger();
    }

    int chargeIdx = mExtendedUi->bat_chargerBox->findData(bCharger);
    if (chargeIdx < 0) chargeIdx = 0; // In case the saved value wasn't found
    mExtendedUi->bat_chargerBox->setCurrentIndex(chargeIdx);

    //////////////////////
    // Battery health
    mExtendedUi->bat_healthBox->clear();
    for (unsigned int idx = 0; idx < NUM_HEALTHS; idx++) {
        mExtendedUi->bat_healthBox->addItem(
                tr(healthAssociations[idx].theString),
                healthAssociations[idx].theEnum );
    }

    BatteryHealth bHealth = BATTERY_HEALTH_UNKNOWN;
    if (mBatteryAgent &&  mBatteryAgent->health) {
        bHealth = mBatteryAgent->health();
    }
    int healthIdx = mExtendedUi->bat_healthBox->findData(bHealth);
    if (healthIdx < 0) healthIdx = 0; // In case the saved value wasn't found
    mExtendedUi->bat_healthBox->setCurrentIndex(healthIdx);

    //////////////////////
    // Battery status
    mExtendedUi->bat_statusBox->clear();
    for (unsigned int idx = 0; idx < NUM_STATUSES; idx++) {
        mExtendedUi->bat_statusBox->addItem(
                tr(statusAssociations[idx].theString),
                statusAssociations[idx].theEnum );
    }

    BatteryStatus bStatus = BATTERY_STATUS_UNKNOWN;
    if (mBatteryAgent &&  mBatteryAgent->status) {
        bStatus = mBatteryAgent->status();
    }
    int statusIdx = mExtendedUi->bat_statusBox->findData(bStatus);
    if (statusIdx < 0) statusIdx = 0; // In case the saved value wasn't found
    mExtendedUi->bat_statusBox->setCurrentIndex(statusIdx);
}

void ExtendedWindow::on_bat_chargerBox_activated(int index)
{
    BatteryCharger bCharger = static_cast<BatteryCharger>(
            mExtendedUi->bat_statusBox->itemData(index).toInt() );

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

void ExtendedWindow::on_bat_healthBox_activated(int index)
{
    BatteryHealth bHealth = static_cast<BatteryHealth>(
            mExtendedUi->bat_healthBox->itemData(index).toInt() );

    if (bHealth >= 0 && bHealth < BATTERY_HEALTH_NUM_ENTRIES) {
        mBatteryState.mHealth = bHealth;

        if (mBatteryAgent && mBatteryAgent->setHealth) {
            mBatteryAgent->setHealth(bHealth);
        }
    }
}

void ExtendedWindow::on_bat_statusBox_activated(int index)
{
    BatteryStatus bStatus = static_cast<BatteryStatus>(
            mExtendedUi->bat_statusBox->itemData(index).toInt() );

    if (bStatus >= 0 && bStatus < BATTERY_STATUS_NUM_ENTRIES) {
        mBatteryState.mStatus = bStatus;

        if (mBatteryAgent && mBatteryAgent->setStatus) {
                mBatteryAgent->setStatus(bStatus);
        }
    }
}
