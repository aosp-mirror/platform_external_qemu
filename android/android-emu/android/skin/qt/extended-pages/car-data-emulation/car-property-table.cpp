// Copyright (C) 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR, A PARTICULAR, PURPOSE.  See the
// GNU General Public License for more details.
#include "android/skin/qt/extended-pages/car-data-emulation/car-property-table.h"

#include "android/skin/qt/qt-settings.h"
#include "android/utils/debug.h"

#include <QObject>
#include <QSettings>
#define D(...) VERBOSE_PRINT(car, __VA_ARGS__)

using std::string;
using std::map;

using emulator::EmulatorMessage;
using emulator::MsgType;
using emulator::Status;
using emulator::VehiclePropValue;

using carpropertyutils::propMap;

static constexpr int LABEL_COLUMN = 0;
static constexpr int VALUE_COLUMN = 1;
static constexpr int AREA_COLUMN = 2;

static map<QString, int> tableIndex;

CarPropertyTable::CarPropertyTable(QWidget* parent)
    : QWidget(parent), mUi(new Ui::CarPropertyTable) {
    mUi->setupUi(this);

    QStringList colHeaders;
    colHeaders << tr("Labels") << tr("Values") << tr("Area");
    mUi->table->setHorizontalHeaderLabels(colHeaders);

    connect(this, SIGNAL(updateData(int, int, QString)),
             this, SLOT(updateTable(int, int, QString)), Qt::QueuedConnection);
    connect(this, SIGNAL(sort(int)), this, SLOT(sortTable(int)), Qt::BlockingQueuedConnection);
    connect(this, SIGNAL(setRowCount(int)), this, SLOT(changeRowCount(int)), Qt::QueuedConnection);
    connect(mUi->table->horizontalHeader(), SIGNAL(sectionClicked(int)),
              this, SLOT(sortTable(int)), Qt::QueuedConnection);
};


// Wrapper slots to call the GUI-modifying functions from callback thread
void CarPropertyTable::sortTable(int column) {
    // Only allowing sorting on Label column, since, for example, sorting
    // by Values (which has different types/meanings) is nonsensical.
    if (column == LABEL_COLUMN) {
        mUi->table->sortItems(column, mNextOrder);
        updateIndices();
        if (mNextOrder == Qt::AscendingOrder) {
            mNextOrder = Qt::DescendingOrder;
        } else {
            mNextOrder = Qt::AscendingOrder;
        }
    }
}

void CarPropertyTable::changeRowCount(int size) {
    mUi->table->setRowCount(size);
}

void CarPropertyTable::updateTable(int row, int col, QString info) {
    mUi->table->setItem(row, col, new QTableWidgetItem(info));
}

void CarPropertyTable::updateIndices() {
    for (int row = 0; row < mUi->table->rowCount(); row++) {
        QTableWidgetItem* currLabel = mUi->table->item(row, LABEL_COLUMN);
        QTableWidgetItem* currArea = mUi->table->item(row, AREA_COLUMN);
        if (currLabel && currArea) {
            tableIndex[currLabel->text() + currArea->text()] = row;
        }
    }
}

void CarPropertyTable::processMsg(EmulatorMessage emulatorMsg) {
    if (emulatorMsg.prop_size() > 0 || emulatorMsg.value_size() > 0) {
        bool addedNewEntries = false;
        if (emulatorMsg.value_size() > mUi->table->rowCount()) {
            emit(setRowCount(emulatorMsg.value_size()));
            addedNewEntries = true;
        }
        for (int valIndex = 0; valIndex < emulatorMsg.value_size(); valIndex++) {
            VehiclePropValue val = emulatorMsg.value(valIndex);
            if (!propMap.count(val.prop())) {
                // Some received constants aren't on the list, so just listing
                // their raw value as their label.
                propMap[val.prop()] = { QString::number(val.prop()) };
            }

            QString label = propMap[val.prop()].label;
            QString value = carpropertyutils::getValueString(val);
            QString area = carpropertyutils::getAreaString(val);

            // Label|Area should be uniquely identifying
            QString key = label + area;

            // If the key already exists, just update the value.
            // Otherwise, create new entry.
            // Also, all entries will be entered on the first message, since we
            // send a getAll request so we don't have to worry about keeping
            // track of next empty row or something like that.
            if (tableIndex.count(key)) {
                emit updateData(tableIndex[key], VALUE_COLUMN, value);
            } else {
                emit updateData(valIndex, LABEL_COLUMN, label);
                emit updateData(valIndex, VALUE_COLUMN, value);
                emit updateData(valIndex, AREA_COLUMN, area);
            }
        }
        if (addedNewEntries) {
            mNextOrder = Qt::AscendingOrder;
            emit sort(0);
        }
    }
}

void CarPropertyTable::setSendEmulatorMsgCallback(CarSensorData::EmulatorMsgCallback&& func) {
    mSendEmulatorMsg = std::move(func);
}

void CarPropertyTable::sendGetAllPropertiesRequest() {
    EmulatorMessage emulatorMsg;
    emulatorMsg.set_msg_type(MsgType::GET_PROPERTY_ALL_CMD);
    emulatorMsg.set_status(Status::RESULT_OK);
    string log = "Requesting all props";
    mSendEmulatorMsg(emulatorMsg, log);
}

void CarPropertyTable::showEvent(QShowEvent*) {
    // Update data when we open the tab
    if (mSendEmulatorMsg != nullptr) { sendGetAllPropertiesRequest(); }
}
