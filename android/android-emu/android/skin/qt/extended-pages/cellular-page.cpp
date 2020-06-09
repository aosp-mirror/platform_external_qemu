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
#include "android/skin/qt/extended-pages/cellular-page.h"

#include <qsettings.h>                                 // for QSettings::Ini...
#include <qstring.h>                                   // for operator+
#include <QComboBox>                                   // for QComboBox
#include <QSettings>                                   // for QSettings
#include <QVariant>                                    // for QVariant

#include "android/avd/util.h"                          // for path_getAvdCon...
#include "android/emulation/VmLock.h"                  // for RecursiveScope...
#include "android/emulation/control/cellular_agent.h"  // for QAndroidCellul...
#include "android/emulator-window.h"                   // for emulator_windo...
#include "android/globals.h"                           // for android_hw
#include "android/main-common.h"                       // for emulator_has_n...
#include "android/skin/qt/qt-settings.h"               // for PER_AVD_SETTIN...
#include "ui_cellular-page.h"                          // for CellularPage

class QWidget;

// Must be protected by the BQL!
static const QAndroidCellularAgent* sCellularAgent = nullptr;
static void saveDataStatus(int status);
static void saveNetworkType(int type);
static void saveSignalStrength(int strength);
static void saveVoiceStatus(int status);
static void saveMeterStatus(int status);
static int  getSavedDataStatus();
static int  getSavedMeterStatus();
static int  getSavedNetworkType();
static int  getSavedSignalStrength();
static int  getSavedVoiceStatus();


CellularPage::CellularPage(QWidget *parent) :
    QWidget(parent),
    mUi(new Ui::CellularPage())
{
    mUi->setupUi(this);

    // Restore previous setting values to the UI widgets

    // Network type
    mUi->cell_standardBox->setCurrentIndex(getSavedNetworkType());

    // Signal strength
    mUi->cell_signalStatusBox->setCurrentIndex(getSavedSignalStrength());

    // Voice status
    mUi->cell_voiceStatusBox->setCurrentIndex(getSavedVoiceStatus());

    // Data status
    mUi->cell_dataStatusBox->setCurrentIndex(getSavedDataStatus());

    // Meter status
    mUi->cell_meterStatusBox->setCurrentIndex(getSavedMeterStatus()); // always metered

}

extern "C" int sim_is_present() {
    return (CellularPage::simIsPresent());
}

// static
bool CellularPage::simIsPresent() {
    EmulatorWindow* const ew = emulator_window_get();
    if (ew) {
        // If the command line says no SIM, we say no SIM.
        if (ew->opts->no_sim) return false;
    }
    // In the absence of UI controls, the default is 'true'.
    return true;
}

// static
void CellularPage::setCellularAgent(const QAndroidCellularAgent* agent) {

    if (!agent) return;

    android::RecursiveScopedVmLock vmlock;

    sCellularAgent = agent;

    // Network parameters

    if (emulator_has_network_option) {
        // The user specified network parameters on the command line.
        // Do not override the command-line values.
        return;
    }

    // Get the settings that were previously saved. Give them
    // to the device.

    // Network type
    if (sCellularAgent->setStandard) {
        sCellularAgent->setStandard((CellularStandard)getSavedNetworkType());
    }

    // Signal strength
    if (sCellularAgent->setSignalStrengthProfile) {
        sCellularAgent->setSignalStrengthProfile((CellularSignal)getSavedSignalStrength());
    }

    // Voice status
    if (sCellularAgent->setVoiceStatus) {
        sCellularAgent->setVoiceStatus((CellularStatus)getSavedVoiceStatus());
    }

    // Data status
    if (sCellularAgent->setDataStatus) {
        sCellularAgent->setDataStatus((CellularStatus)getSavedDataStatus());
    }

    // Meter status
    if (sCellularAgent->setMeterStatus) {
        sCellularAgent->setMeterStatus((CellularMeterStatus)getSavedMeterStatus());
    }
}

void CellularPage::on_cell_standardBox_currentIndexChanged(int index)
{
    saveNetworkType(index);

    android::RecursiveScopedVmLock vmlock;
    if (sCellularAgent && sCellularAgent->setStandard) {
        CellularStandard cStandard = (CellularStandard)index;
        sCellularAgent->setStandard(cStandard);
    }
}

void CellularPage::on_cell_voiceStatusBox_currentIndexChanged(int index)
{
    saveVoiceStatus(index);

    android::RecursiveScopedVmLock vmlock;
    if (sCellularAgent && sCellularAgent->setVoiceStatus) {
        CellularStatus vStatus = (CellularStatus)index;
        sCellularAgent->setVoiceStatus(vStatus);
    }
}

void CellularPage::on_cell_meterStatusBox_currentIndexChanged(int index)
{
    saveMeterStatus(index);

    android::RecursiveScopedVmLock vmlock;
    if (sCellularAgent && sCellularAgent->setMeterStatus) {
        CellularMeterStatus mStatus = (CellularMeterStatus)index;
        sCellularAgent->setMeterStatus(mStatus);
    }
}

void CellularPage::on_cell_dataStatusBox_currentIndexChanged(int index)
{
    saveDataStatus(index);

    android::RecursiveScopedVmLock vmlock;
    if (sCellularAgent && sCellularAgent->setDataStatus) {
        CellularStatus dStatus = (CellularStatus)index;
        sCellularAgent->setDataStatus(dStatus);
    }
}

void CellularPage::on_cell_signalStatusBox_currentIndexChanged(int index)
{
    saveSignalStrength(index);

    android::RecursiveScopedVmLock vmlock;
    if (sCellularAgent && sCellularAgent->setSignalStrengthProfile) {
        CellularSignal signal = (CellularSignal)index;
        sCellularAgent->setSignalStrengthProfile(signal);
    }
}

//////////////
//
// Local static functions to save and retrieve settings

static void saveDataStatus(int status) {
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        avdSpecificSettings.setValue(Ui::Settings::PER_AVD_CELLULAR_DATA_STATUS, status);
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        settings.setValue(Ui::Settings::CELLULAR_DATA_STATUS, status);
    }
}

static void saveNetworkType(int type) {
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        avdSpecificSettings.setValue(Ui::Settings::PER_AVD_CELLULAR_NETWORK_TYPE, type);
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        settings.setValue(Ui::Settings::CELLULAR_NETWORK_TYPE, type);
    }
}

static void saveSignalStrength(int strength) {
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        avdSpecificSettings.setValue(Ui::Settings::PER_AVD_CELLULAR_SIGNAL_STRENGTH, strength);
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        settings.setValue(Ui::Settings::CELLULAR_SIGNAL_STRENGTH, strength);
    }
}

static void saveVoiceStatus(int status) {
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        avdSpecificSettings.setValue(Ui::Settings::PER_AVD_CELLULAR_VOICE_STATUS, status);
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        settings.setValue(Ui::Settings::CELLULAR_VOICE_STATUS, status);
    }
}

static void saveMeterStatus(int status) {
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        avdSpecificSettings.setValue(Ui::Settings::PER_AVD_CELLULAR_METER_STATUS, status);
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        settings.setValue(Ui::Settings::CELLULAR_METER_STATUS, status);
    }
}

static int  getSavedMeterStatus() {
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        return avdSpecificSettings.value(Ui::Settings::PER_AVD_CELLULAR_METER_STATUS,
                                         Cellular_Metered).toInt();
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        return settings.value(Ui::Settings::CELLULAR_METER_STATUS, Cellular_Metered).toInt();
    }
}

static int  getSavedDataStatus() {
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        return avdSpecificSettings.value(Ui::Settings::PER_AVD_CELLULAR_DATA_STATUS,
                                         Cellular_Stat_Home).toInt();
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        return settings.value(Ui::Settings::CELLULAR_DATA_STATUS, Cellular_Stat_Home).toInt();
    }
}

static int  getSavedNetworkType() {
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        return avdSpecificSettings.value(Ui::Settings::PER_AVD_CELLULAR_NETWORK_TYPE,
                                         Cellular_Std_full).toInt();
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        return settings.value(Ui::Settings::CELLULAR_NETWORK_TYPE, Cellular_Std_full).toInt();
    }
}

static int  getSavedSignalStrength() {
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        return avdSpecificSettings.value(Ui::Settings::PER_AVD_CELLULAR_SIGNAL_STRENGTH,
                                         Cellular_Signal_Moderate).toInt();
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        return settings.value(Ui::Settings::CELLULAR_SIGNAL_STRENGTH, Cellular_Signal_Moderate).toInt();
    }
}

static int  getSavedVoiceStatus() {
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        return avdSpecificSettings.value(Ui::Settings::PER_AVD_CELLULAR_VOICE_STATUS,
                                         Cellular_Stat_Home).toInt();
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        return settings.value(Ui::Settings::CELLULAR_VOICE_STATUS, Cellular_Stat_Home).toInt();
    }
}
