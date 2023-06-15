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
#include "android/skin/qt/extended-pages/battery-page-grpc.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QSettings>
#include <QSlider>
#include <QVariant>

#include "android/avd/info.h"
#include "android/avd/util.h"
#include "android/console.h"
#include "android/emulation/control/battery_agent.h"
#include "android/metrics/UiEventTracker.h"
#include "android/skin/qt/qt-settings.h"
#include "android/utils/debug.h"
#include "ui_battery-page-grpc.h"

using android::emulation::control::EmulatorGrpcClient;
using ::google::protobuf::Empty;

#define DEBUG 0
/* set  >1 for very verbose debugging */
#if DEBUG <= 1
#define DD(...) (void)0
#else
#define DD(...) dinfo(__VA_ARGS__)
#endif

static void saveChargeLevel(int chargeLevel);
static void saveCharger(BatteryCharger charger);
static void saveHealth(BatteryHealth health);
static void saveStatus(BatteryStatus status);

static int getSavedChargeLevel();
static BatteryCharger getSavedCharger();
static BatteryHealth getSavedHealth();
static BatteryStatus getSavedStatus();

static void saveBatteryState(BatteryState state);
static BatteryState loadBatteryState();

#define STATE(p) \
    case (p):    \
        s = #p;  \
        break;

static std::string translate_idx(BatteryCharger value) {
    std::string s = "";
    switch (value) {
        STATE(BATTERY_CHARGER_NONE);
        STATE(BATTERY_CHARGER_AC);
        STATE(BATTERY_CHARGER_USB);
        STATE(BATTERY_CHARGER_WIRELESS);
        STATE(BATTERY_CHARGER_NUM_ENTRIES);
    }
    // Chop off BATTERY_
    return s.substr(8);
}
static std::string translate_idx(BatteryHealth value) {
    std::string s = "";
    switch (value) {
        STATE(BATTERY_HEALTH_GOOD);
        STATE(BATTERY_HEALTH_FAILED);
        STATE(BATTERY_HEALTH_DEAD);
        STATE(BATTERY_HEALTH_OVERVOLTAGE);
        STATE(BATTERY_HEALTH_OVERHEATED);
        STATE(BATTERY_HEALTH_UNKNOWN);
        STATE(BATTERY_HEALTH_NUM_ENTRIES);
    }
    // Chop off BATTERY_
    return s.substr(8);
}

static std::string translate_idx(BatteryStatus value) {
    std::string s = "";
    switch (value) {
        STATE(BATTERY_STATUS_UNKNOWN);
        STATE(BATTERY_STATUS_CHARGING);
        STATE(BATTERY_STATUS_DISCHARGING);
        STATE(BATTERY_STATUS_NOT_CHARGING);
        STATE(BATTERY_STATUS_FULL);
        STATE(BATTERY_STATUS_NUM_ENTRIES);
    }
    // Chop off BATTERY_
    return s.substr(8);
}

BatteryPageGrpc::BatteryPageGrpc(QWidget* parent)
    : QWidget(parent),
      mUi(new Ui::BatteryPageGrpc()),
      mDropDownTracker(new UiEventTracker(
              android_studio::EmulatorUiEvent::OPTION_SELECTED,
              android_studio::EmulatorUiEvent::EXTENDED_BATTERY_TAB)),
      mEmulatorControl(EmulatorGrpcClient::me()) {
    mUi->setupUi(this);
    populateListBox(mUi->bat_chargerBox,
                    {
                            {BATTERY_CHARGER_NONE, "None"},
                            {BATTERY_CHARGER_AC, "AC charger"},
                            // TODO: The UI only allows NONE and AC.
                            // Need to also allow and implement USB and
                            // WIRELESS. {BATTERY_CHARGER_USB, "USB charger"},
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

    if (getConsoleAgents()->settings->hw()->hw_battery) {
        mState = loadBatteryState();
        // Update the UI with the saved values
        int chargeLevel = getSavedChargeLevel();

        BatteryCharger batteryCharger = getSavedCharger();
        int chargerIdx = mUi->bat_healthBox->findData(batteryCharger);
        if (chargerIdx < 0)
            chargerIdx =
                    0;  // In case the saved value wasn't found in the pull-down

        BatteryHealth batteryHealth = getSavedHealth();
        int healthIdx = mUi->bat_healthBox->findData(batteryHealth);
        if (healthIdx < 0)
            healthIdx = 0;

        BatteryStatus batteryStatus = getSavedStatus();
        int statusIdx = mUi->bat_statusBox->findData(batteryStatus);
        if (statusIdx < 0)
            statusIdx = 0;

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

static void failureLogger(absl::StatusOr<Empty*> status) {
    if (!status.ok()) {
        dwarning("Failed to set battery due: %s",
                 status.status().message().data());
    }
}
static void saveBatteryState(BatteryState state) {
    saveChargeLevel(state.chargelevel());
    saveCharger((BatteryCharger)state.charger());
    saveHealth((BatteryHealth)state.health());
    saveStatus((BatteryStatus)state.status());
}

static BatteryState loadBatteryState() {
    BatteryState state;
    state.set_hasbattery(true);
    state.set_ispresent(true);
    state.set_chargelevel(getSavedChargeLevel());
    state.set_charger((BatteryState::BatteryCharger)getSavedCharger());
    state.set_health((BatteryState::BatteryHealth)getSavedHealth());
    state.set_status((BatteryState::BatteryStatus)getSavedStatus());
    return state;
}

void BatteryPageGrpc::populateListBox(
        QComboBox* list,
        std::initializer_list<std::pair<int, const char*>> associations) {
    list->clear();
    for (const auto& a : associations) {
        list->addItem(tr(a.second), a.first);
    }
}

void BatteryPageGrpc::on_bat_chargerBox_activated(int index) {
    BatteryCharger bCharger = static_cast<BatteryCharger>(
            mUi->bat_statusBox->itemData(index).toInt());

    if (bCharger >= 0 && bCharger < BATTERY_CHARGER_NUM_ENTRIES) {
        saveCharger(bCharger);
        mState.set_charger((BatteryState::BatteryCharger)bCharger);
        mDropDownTracker->increment(translate_idx(bCharger));
        mEmulatorControl.setBattery(mState, failureLogger);
    }
}

void BatteryPageGrpc::on_bat_levelSlider_valueChanged(int value) {
    // Update the text output
    mUi->bat_chargeLevelText->setText(QString::number(value) + "%");
    mDropDownTracker->increment("LEVEL_SLIDER");
    saveChargeLevel(value);
    mState.set_chargelevel(value);
    mEmulatorControl.setBattery(mState, failureLogger);
}

void BatteryPageGrpc::on_bat_healthBox_activated(int index) {
    BatteryHealth bHealth = static_cast<BatteryHealth>(
            mUi->bat_healthBox->itemData(index).toInt());

    if (bHealth >= 0 && bHealth < BATTERY_HEALTH_NUM_ENTRIES) {
        saveHealth(bHealth);
        mDropDownTracker->increment(translate_idx(bHealth));
        mState.set_health((BatteryState::BatteryHealth)bHealth);
        mEmulatorControl.setBattery(mState, failureLogger);
    }
}

void BatteryPageGrpc::on_bat_statusBox_activated(int index) {
    BatteryStatus bStatus = static_cast<BatteryStatus>(
            mUi->bat_statusBox->itemData(index).toInt());

    if (bStatus >= 0 && bStatus < BATTERY_STATUS_NUM_ENTRIES) {
        saveStatus(bStatus);
        mDropDownTracker->increment(translate_idx(bStatus));
        mState.set_status((BatteryState::BatteryStatus)bStatus);
        mEmulatorControl.setBattery(mState, failureLogger);
    }
}

static void saveChargeLevel(int chargeLevel) {
    int level = std::max<int>(1, chargeLevel);
    const char* avdPath = path_getAvdContentPath(
            getConsoleAgents()->settings->hw()->avd_name);
    if (avdPath) {
        QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        avdSpecificSettings.setValue(Ui::Settings::PER_AVD_BATTERY_CHARGE_LEVEL,
                                     level);
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        settings.setValue(Ui::Settings::BATTERY_CHARGE_LEVEL, level);
    }
}

static void saveCharger(BatteryCharger charger) {
    const char* avdPath = path_getAvdContentPath(
            getConsoleAgents()->settings->hw()->avd_name);
    if (avdPath) {
        QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        avdSpecificSettings.setValue(
                Ui::Settings::PER_AVD_BATTERY_CHARGER_TYPE3, charger);
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        settings.setValue(Ui::Settings::BATTERY_CHARGER_TYPE2, charger);
    }
}

static void saveHealth(BatteryHealth health) {
    const char* avdPath = path_getAvdContentPath(
            getConsoleAgents()->settings->hw()->avd_name);
    if (avdPath) {
        QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        avdSpecificSettings.setValue(Ui::Settings::PER_AVD_BATTERY_HEALTH,
                                     health);
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        settings.setValue(Ui::Settings::BATTERY_HEALTH, health);
    }
}

static void saveStatus(BatteryStatus status) {
    const char* avdPath = path_getAvdContentPath(
            getConsoleAgents()->settings->hw()->avd_name);
    if (avdPath) {
        QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        avdSpecificSettings.setValue(Ui::Settings::PER_AVD_BATTERY_STATUS,
                                     status);
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        settings.setValue(Ui::Settings::BATTERY_STATUS, status);
    }
}

static int getSavedChargeLevel() {
    const char* avdPath = path_getAvdContentPath(
            getConsoleAgents()->settings->hw()->avd_name);
    if (avdPath) {
        QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        return std::max<int>(
                avdSpecificSettings
                        .value(Ui::Settings::PER_AVD_BATTERY_CHARGE_LEVEL, 100)
                        .toInt(),
                1);
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        return std::max<int>(
                settings.value(Ui::Settings::BATTERY_CHARGE_LEVEL, 100).toInt(),
                1);
    }
}

static BatteryCharger getSavedCharger() {
    const char* avdPath = path_getAvdContentPath(
            getConsoleAgents()->settings->hw()->avd_name);
    if (avdPath) {
        QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);

        int noMiscPipe = avdInfo_getApiLevel(
                                 getConsoleAgents()->settings->avdInfo()) < 26;

        BatteryCharger defaultCharger = BATTERY_CHARGER_NONE;

        if (noMiscPipe) {
            defaultCharger = BATTERY_CHARGER_AC;
        }

        // If api level is lower than 26, use ac charging as the default value.
        return (BatteryCharger)avdSpecificSettings
                .value(Ui::Settings::PER_AVD_BATTERY_CHARGER_TYPE3,
                       defaultCharger)
                .toInt();
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        return (BatteryCharger)settings
                .value(Ui::Settings::BATTERY_CHARGER_TYPE2,
                       BATTERY_CHARGER_NONE)
                .toInt();
    }
}

static BatteryHealth getSavedHealth() {
    const char* avdPath = path_getAvdContentPath(
            getConsoleAgents()->settings->hw()->avd_name);
    if (avdPath) {
        QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        return (BatteryHealth)avdSpecificSettings
                .value(Ui::Settings::PER_AVD_BATTERY_HEALTH,
                       BATTERY_HEALTH_GOOD)
                .toInt();
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        return (BatteryHealth)settings
                .value(Ui::Settings::BATTERY_HEALTH, BATTERY_HEALTH_GOOD)
                .toInt();
    }
}

static BatteryStatus getSavedStatus() {
    BatteryCharger batteryCharger = getSavedCharger();
    BatteryStatus defaultBatteryStatus = (batteryCharger == BATTERY_CHARGER_AC)
                                                 ? BATTERY_STATUS_CHARGING
                                                 : BATTERY_STATUS_NOT_CHARGING;

    const char* avdPath = path_getAvdContentPath(
            getConsoleAgents()->settings->hw()->avd_name);
    if (avdPath) {
        QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        return (BatteryStatus)avdSpecificSettings
                .value(Ui::Settings::PER_AVD_BATTERY_HEALTH,
                       defaultBatteryStatus)
                .toInt();
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        return (BatteryStatus)settings
                .value(Ui::Settings::BATTERY_STATUS, defaultBatteryStatus)
                .toInt();
    }
}