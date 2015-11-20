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
#include "android/globals.h"
#include "android/skin/qt/qt-settings.h"
#include "ui_cellular-page.h"

#include <QSettings>

CellularPage::CellularPage(QWidget *parent) :
    QWidget(parent),
    mInitialized(false),
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

    // Set the UI controls according to the saved settings
    QSettings settings;
    QString   avdName(android_hw->avd_name);
    QString   key;

    key = avdName;
    key += "/" + Ui::Settings::CELLULAR_STANDARD;
    CellularStandard cellStd = static_cast<CellularStandard>
            (settings.value(key, Cellular_Std_full).toInt());
    int cellStdIndex = mUi->cell_standardBox->findData(cellStd);
    if (cellStdIndex < 0) cellStdIndex = 0; // In case the value wasn't found
    mUi->cell_standardBox->setCurrentIndex(cellStdIndex);

    key = avdName;
    key += "/" + Ui::Settings::CELLULAR_DATA_STATUS;
    CellularStatus dataStatus = static_cast<CellularStatus>
        (settings.value(key, Cellular_Stat_Home).toInt());
    int dataIndex = mUi->cell_dataStatusBox->findData(dataStatus);
    if (dataIndex < 0) dataIndex = 0;  // In case the value wasn't found
    mUi->cell_dataStatusBox->setCurrentIndex(dataIndex);

    key = avdName;
    key += "/" + Ui::Settings::CELLULAR_VOICE_STATUS;
    CellularStatus voiceStatus = static_cast<CellularStatus>
        (settings.value(key, Cellular_Stat_Home).toInt());
    int voiceIndex = mUi->cell_voiceStatusBox->findData(voiceStatus);
    if (voiceIndex < 0) voiceIndex = 0;  // In case the value wasn't found
    mUi->cell_voiceStatusBox->setCurrentIndex(voiceIndex);
    mInitialized = true;
}

// Initialize the state of the cellular modem according to the
// saved settings
// (static method)

void CellularPage::initCellAvd(const QAndroidCellularAgent* agent) {
    QSettings settings;
    QString   avdName(android_hw->avd_name);
    QString   key;

    if (agent &&  agent->setStandard) {
        key = avdName;
        key += "/" + Ui::Settings::CELLULAR_STANDARD;
        CellularStandard cellStd = static_cast<CellularStandard>
            (settings.value(key, Cellular_Std_full).toInt());
        agent->setStandard(cellStd);
    }

    if (agent &&  agent->setDataStatus) {
        key = avdName;
        key += "/" + Ui::Settings::CELLULAR_DATA_STATUS;
        CellularStatus dataStatus = static_cast<CellularStatus>
            (settings.value(key, Cellular_Stat_Home).toInt());
        agent->setDataStatus(dataStatus);
    }

    if (agent &&  agent->setVoiceStatus) {
        key = avdName;
        key += "/" + Ui::Settings::CELLULAR_VOICE_STATUS;
        CellularStatus voiceStatus = static_cast<CellularStatus>
            (settings.value(key, Cellular_Stat_Home).toInt());
        agent->setVoiceStatus(voiceStatus);
    }
}

void CellularPage::setCellularAgent(const QAndroidCellularAgent* agent) {
    mCellularAgent = agent;
}

void CellularPage::on_cell_standardBox_currentIndexChanged(int index)
{
    if (!mInitialized) return;


    CellularStandard cStandard = static_cast<CellularStandard>(
            mUi->cell_standardBox->itemData(index).toInt() );

    if (cStandard < 0) cStandard = Cellular_Std_full;

    QSettings settings;
    QString   key(android_hw->avd_name);
    key += "/" + Ui::Settings::CELLULAR_STANDARD;
    settings.setValue(key, cStandard);

    if (mCellularAgent && mCellularAgent->setStandard) {
        mCellularAgent->setStandard(cStandard);
    }
}

void CellularPage::on_cell_voiceStatusBox_currentIndexChanged(int index)
{
    if (!mInitialized) return;

    CellularStatus vStatus = static_cast<CellularStatus>(
            mUi->cell_voiceStatusBox->itemData(index).toInt() );

    if (vStatus < 0) vStatus = Cellular_Stat_Home;
    QSettings settings;
    QString   key(android_hw->avd_name);
    key += "/" + Ui::Settings::CELLULAR_VOICE_STATUS;
    settings.setValue(key, vStatus);

    if (mCellularAgent && mCellularAgent->setVoiceStatus) {
        mCellularAgent->setVoiceStatus(vStatus);
    }
}

void CellularPage::on_cell_dataStatusBox_currentIndexChanged(int index)
{
    if (!mInitialized) return;

    CellularStatus dStatus = static_cast<CellularStatus>(
            mUi->cell_voiceStatusBox->itemData(index).toInt() );

    if (dStatus < 0) dStatus = Cellular_Stat_Home;
    QSettings settings;
    QString   key(android_hw->avd_name);
    key += "/" + Ui::Settings::CELLULAR_DATA_STATUS;
    settings.setValue(key, dStatus);

    if (mCellularAgent && mCellularAgent->setDataStatus) {
        mCellularAgent->setDataStatus(dStatus);
    }
}
