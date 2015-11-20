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

#include "android/emulation/control/cellular_agent.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "extended-window.h"
#include "ui_extended.h"

#include <QtWidgets>

void ExtendedWindow::initCellular()
{
    CellularStandard cStandard = Cellular_Std_full;
    // TODO: We have multiple overlapping enums for voice and data
    //       standards (LTE, GSM, etc.). This makes it nearly
    //       impossible to retrieve the standard in use.
    //       This needs to be cleaned up.
    mExtendedUi->cell_standardBox->setCurrentIndex((int)cStandard);

    CellularStatus vStatus = Cellular_Stat_Home;
    if (mCellularAgent && mCellularAgent->voiceStatus) {
        vStatus = mCellularAgent->voiceStatus();
    }
    mExtendedUi->cell_voiceStatusBox->setCurrentIndex((int)vStatus);

    CellularStatus dStatus = Cellular_Stat_Home;
    if (mCellularAgent && mCellularAgent->dataStatus) {
        dStatus = mCellularAgent->dataStatus();
    }
    mExtendedUi->cell_dataStatusBox->setCurrentIndex((int)dStatus);

    // TODO: Network delay
}

void ExtendedWindow::on_cell_standardBox_currentIndexChanged(int index)
{
    if (mCellularAgent && mCellularAgent->setStandard) {
        CellularStandard cStandard = (CellularStandard)index;
        mCellularAgent->setStandard(cStandard);
    }
}

void ExtendedWindow::on_cell_delayBox_currentIndexChanged(int index)
{
    // See http://developer.android.com/tools/devices/emulator.html#netdelay

    // TODO: Implement Network delay setting
    printf("===== ext-cellular.cpp: The Network delay setting is not implemented!\n"); // ??
}

void ExtendedWindow::on_cell_voiceStatusBox_currentIndexChanged(int index)
{
    if (mCellularAgent && mCellularAgent->setVoiceStatus) {
        CellularStatus vStatus = (CellularStatus)index;
        mCellularAgent->setVoiceStatus(vStatus);
    }
}

void ExtendedWindow::on_cell_dataStatusBox_currentIndexChanged(int index)
{
    if (mCellularAgent && mCellularAgent->setDataStatus) {
        CellularStatus dStatus = (CellularStatus)index;
        mCellularAgent->setDataStatus(dStatus);
    }
}
