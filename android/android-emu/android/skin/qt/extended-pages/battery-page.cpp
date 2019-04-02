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

static void saveChargeLevel(int chargeLevel);
static void saveCharger(BatteryCharger charger);
static void saveHealth(BatteryHealth health);
static void saveStatus(BatteryStatus status);

static int            getSavedChargeLevel();
static BatteryCharger getSavedCharger();
static BatteryHealth  getSavedHealth();
static BatteryStatus  getSavedStatus();

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
        int chargeLevel = getSavedChargeLevel();

        BatteryCharger batteryCharger = getSavedCharger();
        int chargerIdx = mUi->bat_healthBox->findData(batteryCharger);
        if (chargerIdx < 0) chargerIdx = 0;  // In case the saved value wasn't found in the pull-down

        BatteryHealth batteryHealth = getSavedHealth();
        int healthIdx = mUi->bat_healthBox->findData(batteryHealth);
        if (healthIdx < 0) healthIdx = 0;

        BatteryStatus batteryStatus = getSavedStatus();
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
    sBatteryAgent = agent;

    if (sBatteryAgent) {
        // Send the current settings to the device
        if (sBatteryAgent->setChargeLevel) {
            sBatteryAgent->setChargeLevel(getSavedChargeLevel());
        }

        if (sBatteryAgent->setCharger) {
            sBatteryAgent->setCharger(getSavedCharger());
        }

        if (sBatteryAgent->health) {
            sBatteryAgent->setHealth(getSavedHealth());
        }

        if (sBatteryAgent->setStatus) {
            sBatteryAgent->setStatus(getSavedStatus());
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

    saveCharger(bCharger);

    if (bCharger >= 0 && bCharger < BATTERY_CHARGER_NUM_ENTRIES) {
        if (sBatteryAgent && sBatteryAgent->setCharger) {
            sBatteryAgent->setCharger(bCharger);
        }
    }
}

void BatteryPage::on_bat_levelSlider_valueChanged(int value) {
    // Update the text output
    mUi->bat_chargeLevelText->setText(QString::number(value) + "%");

    saveChargeLevel(value);

    if (sBatteryAgent && sBatteryAgent->setChargeLevel) {
        sBatteryAgent->setChargeLevel(value);
    }
}

void BatteryPage::on_bat_healthBox_activated(int index) {
    BatteryHealth bHealth = static_cast<BatteryHealth>(
            mUi->bat_healthBox->itemData(index).toInt());

    saveHealth(bHealth);

    if (bHealth >= 0 && bHealth < BATTERY_HEALTH_NUM_ENTRIES) {
        if (sBatteryAgent && sBatteryAgent->setHealth) {
            sBatteryAgent->setHealth(bHealth);
        }
    }
}

void BatteryPage::on_bat_statusBox_activated(int index) {
    BatteryStatus bStatus = static_cast<BatteryStatus>(
            mUi->bat_statusBox->itemData(index).toInt());

    saveStatus(bStatus);

    if (bStatus >= 0 && bStatus < BATTERY_STATUS_NUM_ENTRIES) {
        if (sBatteryAgent && sBatteryAgent->setStatus) {
            sBatteryAgent->setStatus(bStatus);
        }
    }
}

static void saveChargeLevel(int chargeLevel) {
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        avdSpecificSettings.setValue(Ui::Settings::PER_AVD_BATTERY_CHARGE_LEVEL, chargeLevel);
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        settings.setValue(Ui::Settings::BATTERY_CHARGE_LEVEL, chargeLevel);
    }
}

static void saveCharger(BatteryCharger charger) {
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        avdSpecificSettings.setValue(Ui::Settings::PER_AVD_BATTERY_CHARGER_TYPE3, charger);
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        settings.setValue(Ui::Settings::BATTERY_CHARGER_TYPE2, charger);
    }
}

static void saveHealth(BatteryHealth health) {
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        avdSpecificSettings.setValue(Ui::Settings::PER_AVD_BATTERY_HEALTH, health);
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        settings.setValue(Ui::Settings::BATTERY_HEALTH, health);
    }
}

static void saveStatus(BatteryStatus status) {
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        avdSpecificSettings.setValue(Ui::Settings::PER_AVD_BATTERY_STATUS, status);
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        settings.setValue(Ui::Settings::BATTERY_STATUS, status);
    }
}

static int getSavedChargeLevel() {
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        return avdSpecificSettings.value(Ui::Settings::PER_AVD_BATTERY_CHARGE_LEVEL, 100).toInt();
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        return settings.value(Ui::Settings::BATTERY_CHARGE_LEVEL, 100).toInt();
    }
}

static BatteryCharger getSavedCharger() {
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);

        int noMiscPipe = avdInfo_getApiLevel(android_avdInfo) < 26;

        BatteryCharger defaultCharger = BATTERY_CHARGER_NONE;

        if (noMiscPipe) {
            defaultCharger = BATTERY_CHARGER_AC;
        }

        // If api level is lower than 26, use ac charging as the default value.
        return (BatteryCharger)avdSpecificSettings.value(
                                   Ui::Settings::PER_AVD_BATTERY_CHARGER_TYPE3,
                                   defaultCharger).toInt();
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        return (BatteryCharger)settings.value(Ui::Settings::BATTERY_CHARGER_TYPE2,
                                              BATTERY_CHARGER_NONE).toInt();
    }
}

static BatteryHealth getSavedHealth() {
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        return (BatteryHealth)avdSpecificSettings.value(
                                   Ui::Settings::PER_AVD_BATTERY_HEALTH,
                                   BATTERY_HEALTH_GOOD).toInt();
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        return (BatteryHealth)settings.value(Ui::Settings::BATTERY_HEALTH,
                                             BATTERY_HEALTH_GOOD).toInt();
    }
}

static BatteryStatus getSavedStatus() {
    BatteryCharger batteryCharger = getSavedCharger();
    BatteryStatus defaultBatteryStatus = (batteryCharger == BATTERY_CHARGER_AC) ?
                                               BATTERY_STATUS_CHARGING :
                                               BATTERY_STATUS_NOT_CHARGING;

    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        return (BatteryStatus)avdSpecificSettings.value(
                                   Ui::Settings::PER_AVD_BATTERY_HEALTH,
                                   defaultBatteryStatus).toInt();
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        return (BatteryStatus)settings.value(Ui::Settings::BATTERY_STATUS,
                                             defaultBatteryStatus).toInt();
    }
}
