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
#include "android/skin/qt/extended-pages/battery-page.h"

#include "android/emulation/control/battery_agent.h"
#include "android/emulation/VmLock.h"
#include "android/globals.h"
#include "android/skin/qt/qt-settings.h"

#include "ui_battery-page.h"

#include <QSettings>

// Must be protected by the BQL!
static const QAndroidBatteryAgent* sBatteryAgent = nullptr;

BatteryPage::BatteryPage(QWidget* parent)
    : QWidget(parent), mUi(new Ui::BatteryPage()) {
    mUi->setupUi(this);
    populateListBox(
            mUi->bat_chargerBox,
            {
                    {BATTERY_CHARGER_NONE, "None"},
                    {BATTERY_CHARGER_AC, "AC charger"},
                    // TODO: The UI only allows NONE and AC.
                    // Need to also allow and implement USB and WIRELESS.
                    // {BATTERY_CHARGER_USB, "USB charger"},
                    // {BATTERY_CHARGER_WIRELESS, "Wireless charger"},
            });
    populateListBox(mUi->bat_healthBox,
                    {
                            {BATTERY_HEALTH_GOOD, "Good"},
                            {BATTERY_HEALTH_FAILED, "Failed"},
                            {BATTERY_HEALTH_DEAD, "Dead"},
                            {BATTERY_HEALTH_OVERVOLTAGE, "Overvoltage"},
                            {BATTERY_HEALTH_OVERHEATED, "Overheated"},
                            {BATTERY_HEALTH_UNKNOWN, "Unknown"},
                    });
    populateListBox(mUi->bat_statusBox,
                    {
                            {BATTERY_STATUS_UNKNOWN, "Unknown"},
                            {BATTERY_STATUS_CHARGING, "Charging"},
                            {BATTERY_STATUS_DISCHARGING, "Discharging"},
                            {BATTERY_STATUS_NOT_CHARGING, "Not charging"},
                            {BATTERY_STATUS_FULL, "Full"},
                    });

    if (android_hw->hw_battery) {
        // Update the UI with the saved values
        QSettings settings;
        int chargeLevel = settings.value(Ui::Settings::BATTERY_CHARGE_LEVEL, 100).toInt();

        BatteryCharger batteryCharger = (BatteryCharger)settings.value(
                                                      Ui::Settings::BATTERY_CHARGER_TYPE,
                                                      BATTERY_CHARGER_AC).toInt();
        int chargerIdx = mUi->bat_healthBox->findData(batteryCharger);
        if (chargerIdx < 0) chargerIdx = 0;  // In case the saved value wasn't found in the pull-down

        BatteryHealth batteryHealth = (BatteryHealth)settings.value(Ui::Settings::BATTERY_HEALTH,
                                                                    BATTERY_HEALTH_GOOD).toInt();
        int healthIdx = mUi->bat_healthBox->findData(batteryHealth);
        if (healthIdx < 0) healthIdx = 0;

        BatteryStatus defaultBatteryStatus = (batteryCharger == BATTERY_CHARGER_AC) ?
                                                   BATTERY_STATUS_CHARGING :
                                                   BATTERY_STATUS_NOT_CHARGING;
        BatteryStatus batteryStatus = (BatteryStatus)settings.value(Ui::Settings::BATTERY_STATUS,
                                                                    defaultBatteryStatus).toInt();
        int statusIdx = mUi->bat_statusBox->findData(batteryStatus);
        if (statusIdx < 0) statusIdx = 0;

        mUi->bat_levelSlider->setValue(chargeLevel);
        mUi->bat_chargeLevelText->setText(QString::number(chargeLevel) + "%");
        mUi->bat_chargerBox->setCurrentIndex(chargerIdx);
        mUi->bat_healthBox->setCurrentIndex(healthIdx);
        mUi->bat_statusBox->setCurrentIndex(statusIdx);

        // Don't show the overlay that obscures the buttons.
        // Don't show the message saying that there is no battery.
        mUi->bat_noBat_mask->hide();
        mUi->bat_noBat_message->hide();
    } else {
        // The device has no battery. Show the "no battery" message.
        mUi->bat_noBat_mask->raise();
        mUi->bat_noBat_message->raise();
    }
}

// static
void BatteryPage::setBatteryAgent(const QAndroidBatteryAgent* agent) {
    if (!android_hw->hw_battery) {
        // This device has no battery. Ignore the agent.
        return;
    }

    // The VM lock is needed because the battery agent touches the
    // goldfish battery virtual device explicitly
    android::RecursiveScopedVmLock vmlock;
    sBatteryAgent = agent;

    if (sBatteryAgent) {
        // Send the current settings to the device
        QSettings settings;
        if (sBatteryAgent->setChargeLevel) {
            int chargeLevel = settings.value(Ui::Settings::BATTERY_CHARGE_LEVEL,
                                             100).toInt();
            sBatteryAgent->setChargeLevel(chargeLevel);
        }

        BatteryCharger batteryCharger = (BatteryCharger)settings.value(
                                                      Ui::Settings::BATTERY_CHARGER_TYPE,
                                                      BATTERY_CHARGER_AC).toInt();
        if (sBatteryAgent->setCharger) {
            sBatteryAgent->setCharger(batteryCharger);
        }

        if (sBatteryAgent->health) {
            BatteryHealth batteryHealth = (BatteryHealth)settings.value(
                                               Ui::Settings::BATTERY_HEALTH,
                                               BATTERY_HEALTH_GOOD).toInt();
            sBatteryAgent->setHealth(batteryHealth);
        }

        if (sBatteryAgent->setStatus) {
            BatteryStatus defaultBatteryStatus = (batteryCharger == BATTERY_CHARGER_AC) ?
                                                       BATTERY_STATUS_CHARGING :
                                                       BATTERY_STATUS_NOT_CHARGING;
            BatteryStatus batteryStatus = (BatteryStatus)settings.value(
                                               Ui::Settings::BATTERY_STATUS,
                                               defaultBatteryStatus).toInt();
            sBatteryAgent->setStatus(batteryStatus);
        }
    }
}

void BatteryPage::populateListBox(
        QComboBox* list,
        std::initializer_list<std::pair<int, const char*>> associations) {
    list->clear();
    for (const auto& a : associations) {
        list->addItem(tr(a.second), a.first);
    }
}

void BatteryPage::on_bat_chargerBox_activated(int index) {
    BatteryCharger bCharger = static_cast<BatteryCharger>(
            mUi->bat_statusBox->itemData(index).toInt());

    QSettings settings;
    settings.setValue(Ui::Settings::BATTERY_CHARGER_TYPE, bCharger);

    android::RecursiveScopedVmLock vmlock;
    if (bCharger >= 0 && bCharger < BATTERY_CHARGER_NUM_ENTRIES) {
        if (sBatteryAgent && sBatteryAgent->setCharger) {
            sBatteryAgent->setCharger(bCharger);
        }
    }
}

void BatteryPage::on_bat_levelSlider_valueChanged(int value) {
    // Update the text output
    mUi->bat_chargeLevelText->setText(QString::number(value) + "%");

    QSettings settings;
    settings.setValue(Ui::Settings::BATTERY_CHARGE_LEVEL, value);

    android::RecursiveScopedVmLock vmlock;
    if (sBatteryAgent && sBatteryAgent->setChargeLevel) {
        sBatteryAgent->setChargeLevel(value);
    }
}

void BatteryPage::on_bat_healthBox_activated(int index) {
    BatteryHealth bHealth = static_cast<BatteryHealth>(
            mUi->bat_healthBox->itemData(index).toInt());

    QSettings settings;
    settings.setValue(Ui::Settings::BATTERY_HEALTH, bHealth);

    android::RecursiveScopedVmLock vmlock;
    if (bHealth >= 0 && bHealth < BATTERY_HEALTH_NUM_ENTRIES) {
        if (sBatteryAgent && sBatteryAgent->setHealth) {
            sBatteryAgent->setHealth(bHealth);
        }
    }
}

void BatteryPage::on_bat_statusBox_activated(int index) {
    BatteryStatus bStatus = static_cast<BatteryStatus>(
            mUi->bat_statusBox->itemData(index).toInt());

    QSettings settings;
    settings.setValue(Ui::Settings::BATTERY_STATUS, bStatus);

    android::RecursiveScopedVmLock vmlock;
    if (bStatus >= 0 && bStatus < BATTERY_STATUS_NUM_ENTRIES) {
        if (sBatteryAgent && sBatteryAgent->setStatus) {
            sBatteryAgent->setStatus(bStatus);
        }
    }
}
