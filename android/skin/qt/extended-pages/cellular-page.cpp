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
    static std::vector<std::pair<enum CellularStandard,
                                 std::string>          > cellStdAssociations =
    {
        { Cellular_Std_GSM,    "GSM"   },
        { Cellular_Std_HSCSD,  "HSCSD" },
        { Cellular_Std_GPRS,   "GPRS"  },
        { Cellular_Std_EDGE,   "EDGE"  },
        { Cellular_Std_UMTS,   "UMTS"  },
        { Cellular_Std_HSDPA,  "HSDPA" },
        { Cellular_Std_full,   "full"  },
    };

    static std::vector<std::pair<enum CellularStatus,
                                 std::string>        > voiceStatusAssociations =
    {
        { Cellular_Stat_Home,         "Home"                          },
        { Cellular_Stat_Roaming,      "Roaming"                       },
        { Cellular_Stat_Searching,    "Searching"                     },
        { Cellular_Stat_Denied,       "Denied (emergency calls only)" },
        { Cellular_Stat_Unregistered, "Unregisterd (off)"             },
    };

    static std::vector<std::pair<enum CellularStatus,
                       std::string>                  > dataStatusAssociations =
    {
        { Cellular_Stat_Home,         "Home"              },
        { Cellular_Stat_Roaming,      "Roaming"           },
        { Cellular_Stat_Searching,    "Searching"         },
        { Cellular_Stat_Denied,       "Denied"            },
        { Cellular_Stat_Unregistered, "Unregisterd (off)" },
    };

    mUi->setupUi(this);

    populateListBox(cellStdAssociations,     mUi->cell_standardBox);
    populateListBox(voiceStatusAssociations, mUi->cell_voiceStatusBox);
    populateListBox(dataStatusAssociations,  mUi->cell_dataStatusBox);
}

void CellularPage::setCellularAgent(const QAndroidCellularAgent* agent) {
    mCellularAgent = agent;

    // Initialize the UI controls to match the device's current settings
    CellularStandard cellStd = Cellular_Std_full;
    if (mCellularAgent && mCellularAgent->standard) {
        cellStd = mCellularAgent->standard();
    }
    int cellStdIndex = mUi->cell_standardBox->findData(cellStd);
    if (cellStdIndex < 0) cellStdIndex = 0; // In case the value wasn't found
    mUi->cell_standardBox->setCurrentIndex(cellStdIndex);

    CellularStatus voiceStatus = Cellular_Stat_Home;
    if (mCellularAgent && mCellularAgent->voiceStatus) {
        voiceStatus = mCellularAgent->voiceStatus();
    }
    int voiceIndex = mUi->cell_voiceStatusBox->findData(voiceStatus);
    if (voiceIndex < 0) voiceIndex = 0;  // In case the value wasn't found
    mUi->cell_voiceStatusBox->setCurrentIndex(voiceIndex);

    CellularStatus dataStatus = Cellular_Stat_Home;
    if (mCellularAgent && mCellularAgent->dataStatus) {
        dataStatus = mCellularAgent->dataStatus();
    }
    int dataIndex = mUi->cell_dataStatusBox->findData(dataStatus);
    if (dataIndex < 0) dataIndex = 0;  // In case the value wasn't found
    mUi->cell_dataStatusBox->setCurrentIndex(dataIndex);
}

void CellularPage::on_cell_standardBox_currentIndexChanged(int index)
{
    if (mCellularAgent && mCellularAgent->setStandard) {
        CellularStandard cStandard = (CellularStandard)index;
        mCellularAgent->setStandard(cStandard);
    }
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
