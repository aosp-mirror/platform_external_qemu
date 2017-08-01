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
#include "android/emulator-window.h"
#include "android/globals.h"
#include "android/main-common.h"
#include "android/skin/qt/qt-settings.h"
#include "ui_cellular-page.h"

#include <QMessageBox>
#include <QSettings>

CellularPage::CellularPage(QWidget *parent) :
    QWidget(parent),
    mUi(new Ui::CellularPage())
{
    mUi->setupUi(this);

    // Restore previous values
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

    // SIM present
    mUi->cell_simBox->setChecked(simIsPresent());
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

    return simSetting();
}

// static
bool CellularPage::simSetting() {
    QSettings settings;

    // Set the SIM present or absent
    QString simKey(android_hw->avd_name);
    simKey += "/";
    simKey += Ui::Settings::CELLULAR_SIM_PRESENT;
    return settings.value(simKey, true).toBool();
}

void CellularPage::setCellularAgent(const QAndroidCellularAgent* agent) {

    if (!agent) return;

    mCellularAgent = agent;

    QSettings settings;

    // Tell the device if a SIM is present
    if (mCellularAgent->setSimPresent) {
        mCellularAgent->setSimPresent(simIsPresent());
    }

    // Network parameters

    if (emulator_has_network_option) {
        // The user specified network parameters on the command line.
        // Do not override the command-line values.
        return;
    }

    // Get the settings that were previously saved. Give them
    // to the device.

    // Network type
    int cStandard = settings.value(Ui::Settings::CELLULAR_NETWORK_TYPE,
                                   Cellular_Std_full).toInt();
    mCellularAgent->setStandard((CellularStandard)cStandard);

    // Signal strength
    int cStrength = settings.value(Ui::Settings::CELLULAR_SIGNAL_STRENGTH,
                                   Cellular_Signal_Moderate).toInt();
    mCellularAgent->setSignalStrengthProfile((CellularSignal)cStrength);

    // Voice status
    int voiceStatus = settings.value(Ui::Settings::CELLULAR_VOICE_STATUS,
                                     Cellular_Stat_Home).toInt();
    mCellularAgent->setVoiceStatus((CellularStatus)voiceStatus);

    // Data status
    int dataStatus = settings.value(Ui::Settings::CELLULAR_DATA_STATUS,
                                    Cellular_Stat_Home).toInt();
    mCellularAgent->setDataStatus((CellularStatus)dataStatus);
}

void CellularPage::on_cell_standardBox_currentIndexChanged(int index)
{
    QSettings settings;
    settings.setValue(Ui::Settings::CELLULAR_NETWORK_TYPE, index);

    if (mCellularAgent && mCellularAgent->setStandard) {
        CellularStandard cStandard = (CellularStandard)index;
        mCellularAgent->setStandard(cStandard);
    }
}

void CellularPage::on_cell_voiceStatusBox_currentIndexChanged(int index)
{
    QSettings settings;
    settings.setValue(Ui::Settings::CELLULAR_VOICE_STATUS, index);

    if (mCellularAgent && mCellularAgent->setVoiceStatus) {
        CellularStatus vStatus = (CellularStatus)index;
        mCellularAgent->setVoiceStatus(vStatus);
    }
}

void CellularPage::on_cell_dataStatusBox_currentIndexChanged(int index)
{
    QSettings settings;
    settings.setValue(Ui::Settings::CELLULAR_DATA_STATUS, index);

    if (mCellularAgent && mCellularAgent->setDataStatus) {
        CellularStatus dStatus = (CellularStatus)index;
        mCellularAgent->setDataStatus(dStatus);
    }
}

void CellularPage::on_cell_signalStatusBox_currentIndexChanged(int index)
{
    QSettings settings;
    settings.setValue(Ui::Settings::CELLULAR_SIGNAL_STRENGTH, index);

    if (mCellularAgent && mCellularAgent->setSignalStrengthProfile) {
        CellularSignal signal = (CellularSignal)index;
        mCellularAgent->setSignalStrengthProfile(signal);
    }
}

void CellularPage::on_cell_simBox_clicked(bool isPresent)
{
    // Save the new setting
    QSettings settings;
    QString simKey(android_hw->avd_name);
    simKey += "/";
    simKey += Ui::Settings::CELLULAR_SIM_PRESENT;
    settings.setValue(simKey, isPresent);

    // Tell the device
    if (mCellularAgent && mCellularAgent->setSimPresent) {
        mCellularAgent->setSimPresent(isPresent);
    }
    // Note: The device will see this change only after
    //       airplane mode is toggled off then on.
    //       We cannot perform this toggle here because the
    //       Android system restricts who is allowed to broadcast
    //       the AIRPLANE_MODE intent.

    if (!mSimChangeInfoWasShown) {
        mSimChangeInfoWasShown = true;
        QMessageBox infoBox;
        infoBox.setText(tr("For the SIM change to take effect, you must either "
                           "toggle airplane mode or reboot the device."));
        infoBox.setIcon(QMessageBox::Information);
        infoBox.addButton(tr("Got it"), QMessageBox::YesRole);
        infoBox.exec();
    }
}
