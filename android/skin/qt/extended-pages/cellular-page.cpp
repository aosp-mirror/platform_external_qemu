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
#include "ui_cellular-page.h"

CellularPage::CellularPage(QWidget *parent) :
    QWidget(parent),
    mUi(new Ui::CellularPage()),
    mCellularAgent(nullptr)
{
    mUi->setupUi(this);
}

void CellularPage::setCellularAgent(const QAndroidCellularAgent* agent) {
    mCellularAgent = agent;
}
void CellularPage::on_cell_standardBox_currentIndexChanged(int index)
{
    if (mCellularAgent && mCellularAgent->setStandard) {
        CellularStandard cStandard = (CellularStandard)index;
        mCellularAgent->setStandard(cStandard);
    }
}

void CellularPage::on_cell_delayBox_currentIndexChanged(int index)
{
    // See http://developer.android.com/tools/devices/emulator.html#netdelay
    // TODO: Implement Network delay setting
    printf("===== ext-cellular.cpp: The Network delay setting is not implemented!\n");
}

void CellularPage::on_cell_voiceStatusBox_currentIndexChanged(int index)
{
    if (mCellularAgent && mCellularAgent->setVoiceStatus) {
        CellularStatus vStatus = (CellularStatus)index;
        mCellularAgent->setVoiceStatus(vStatus);
    }
}

void CellularPage::on_cell_dataStatusBox_currentIndexChanged(int index)
{
    if (mCellularAgent && mCellularAgent->setDataStatus) {
        CellularStatus dStatus = (CellularStatus)index;
        mCellularAgent->setDataStatus(dStatus);
    }
}