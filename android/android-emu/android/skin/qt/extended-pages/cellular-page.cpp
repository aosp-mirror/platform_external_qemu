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

#include "android/emulation/control/cellular_agent.h"
#include "android/emulation/VmLock.h"
#include "android/emulator-window.h"
#include "android/main-common.h"
#include "android/skin/qt/qt-settings.h"
#include "ui_cellular-page.h"

#include <QSettings>

// Must be protected by the BQL!
static const QAndroidCellularAgent* sCellularAgent = nullptr;

CellularPage::CellularPage(QWidget *parent) :
    QWidget(parent),
    mUi(new Ui::CellularPage())
{
    mUi->setupUi(this);

    // Restore previous setting values to the UI widgets
    QSettings settings;

    // Network type
    int cStandard = settings.value(Ui::Settings::CELLULAR_NETWORK_TYPE,
                                   Cellular_Std_full).toInt();
    mUi->cell_standardBox->setCurrentIndex(cStandard);

    // Signal strength
    int cStrength = settings.value(Ui::Settings::CELLULAR_SIGNAL_STRENGTH,
                                   Cellular_Signal_Moderate).toInt();
    mUi->cell_signalStatusBox->setCurrentIndex(cStrength);

    // Voice status
    int voiceStatus = settings.value(Ui::Settings::CELLULAR_VOICE_STATUS,
                                     Cellular_Stat_Home).toInt();
    mUi->cell_voiceStatusBox->setCurrentIndex(voiceStatus);

    // Data status
    int dataStatus = settings.value(Ui::Settings::CELLULAR_DATA_STATUS,
                                    Cellular_Stat_Home).toInt();
    mUi->cell_dataStatusBox->setCurrentIndex(dataStatus);
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

    QSettings settings;

    // Network type
    if (sCellularAgent->setStandard) {
        int cStandard = settings.value(Ui::Settings::CELLULAR_NETWORK_TYPE,
                                       Cellular_Std_full).toInt();
        sCellularAgent->setStandard((CellularStandard)cStandard);
    }

    // Signal strength
    if (sCellularAgent->setSignalStrengthProfile) {
        int cStrength = settings.value(Ui::Settings::CELLULAR_SIGNAL_STRENGTH,
                                       Cellular_Signal_Moderate).toInt();
        sCellularAgent->setSignalStrengthProfile((CellularSignal)cStrength);
    }

    // Voice status
    if (sCellularAgent->setVoiceStatus) {
        int voiceStatus = settings.value(Ui::Settings::CELLULAR_VOICE_STATUS,
                                         Cellular_Stat_Home).toInt();
        sCellularAgent->setVoiceStatus((CellularStatus)voiceStatus);
    }

    // Data status
    if (sCellularAgent->setDataStatus) {
        int dataStatus = settings.value(Ui::Settings::CELLULAR_DATA_STATUS,
                                        Cellular_Stat_Home).toInt();
        sCellularAgent->setDataStatus((CellularStatus)dataStatus);
    }
}

void CellularPage::on_cell_standardBox_currentIndexChanged(int index)
{
    QSettings settings;
    settings.setValue(Ui::Settings::CELLULAR_NETWORK_TYPE, index);

    android::RecursiveScopedVmLock vmlock;
    if (sCellularAgent && sCellularAgent->setStandard) {
        CellularStandard cStandard = (CellularStandard)index;
        sCellularAgent->setStandard(cStandard);
    }
}

void CellularPage::on_cell_voiceStatusBox_currentIndexChanged(int index)
{
    QSettings settings;
    settings.setValue(Ui::Settings::CELLULAR_VOICE_STATUS, index);

    android::RecursiveScopedVmLock vmlock;
    if (sCellularAgent && sCellularAgent->setVoiceStatus) {
        CellularStatus vStatus = (CellularStatus)index;
        sCellularAgent->setVoiceStatus(vStatus);
    }
}

void CellularPage::on_cell_dataStatusBox_currentIndexChanged(int index)
{
    QSettings settings;
    settings.setValue(Ui::Settings::CELLULAR_DATA_STATUS, index);

    android::RecursiveScopedVmLock vmlock;
    if (sCellularAgent && sCellularAgent->setDataStatus) {
        CellularStatus dStatus = (CellularStatus)index;
        sCellularAgent->setDataStatus(dStatus);
    }
}

void CellularPage::on_cell_signalStatusBox_currentIndexChanged(int index)
{
    QSettings settings;
    settings.setValue(Ui::Settings::CELLULAR_SIGNAL_STRENGTH, index);

    android::RecursiveScopedVmLock vmlock;
    if (sCellularAgent && sCellularAgent->setSignalStrengthProfile) {
        CellularSignal signal = (CellularSignal)index;
        sCellularAgent->setSignalStrengthProfile(signal);
    }
}
