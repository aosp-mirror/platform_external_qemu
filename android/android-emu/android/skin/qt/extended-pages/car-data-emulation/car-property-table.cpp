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
#include "android/skin/qt/extended-pages/car-data-emulation/vehicle_constants_generated.h"
#include "android/skin/qt/extended-pages/car-data-emulation/checkbox-dialog.h"

#include <cfloat>

#include <QObject>
#include <QSettings>
#include <QInputDialog>
#define D(...) VERBOSE_PRINT(car, __VA_ARGS__)

using std::string;
using std::map;
using std::vector;

using emulator::EmulatorMessage;
using emulator::MsgType;
using emulator::Status;
using emulator::VehiclePropValue;
using emulator::VehiclePropertyType;
using emulator::VehiclePropGet;

using carpropertyutils::PropertyDescription;
using carpropertyutils::propMap;

static constexpr int LABEL_COLUMN = 0;
static constexpr int VALUE_COLUMN = 1;
static constexpr int AREA_COLUMN = 2;
static constexpr int PROPERTY_ID_COLUMN = 3;
static constexpr int AREA_ID_COLUMN = 4;
static constexpr int TYPE_COLUMN = 5;

static constexpr int REFRESH_START = 1;
static constexpr int REFRESH_STOP = 2;
static constexpr int REFRESH_PUASE = 3;

static constexpr int64_t REFRESH_INTERVEL = 1000000LL;

static map<QString, int> tableIndex;

CarPropertyTable::CarPropertyTable(QWidget* parent)
    : QWidget(parent),
      mUi(new Ui::CarPropertyTable),
      mCarPropertyTableRefreshThread([this] {
          int msg;
          while (true) {
              mRefreshMsg.tryReceive(&msg);
              if (msg == REFRESH_STOP) {
                  break;
              }
              android::base::AutoLock lock(mCarPropertyTableRefreshLock);
              switch (msg) {
                  case REFRESH_START:
                      if (mSendEmulatorMsg != nullptr) {
                          sendGetAllPropertiesRequest();
                      }
                      mCarPropertyTableRefreshCV.timedWait(
                              &mCarPropertyTableRefreshLock,
                              nextRefreshAbsolute());
                      break;
                  case REFRESH_PUASE:
                      mCarPropertyTableRefreshCV.wait(
                              &mCarPropertyTableRefreshLock);
                      break;
              }
          }
      }) {
    mUi->setupUi(this);

    QStringList colHeaders;
    colHeaders << tr("Labels") << tr("Values") << tr("Area")
               << tr("Property IDs") << tr("Area IDs") << tr("Type");
    // This "hidden" data is necessary when sending changes.
    mUi->table->setColumnHidden(AREA_ID_COLUMN, true);
    mUi->table->setColumnHidden(TYPE_COLUMN, true);
    mUi->table->setHorizontalHeaderLabels(colHeaders);

    connect(this, SIGNAL(updateData(int, int, QTableWidgetItem*)), this,
            SLOT(updateTable(int, int, QTableWidgetItem*)),
            Qt::QueuedConnection);
    connect(this, SIGNAL(sort(int)), this, SLOT(sortTable(int)), Qt::BlockingQueuedConnection);
    connect(this, SIGNAL(setRowCount(int)), this, SLOT(changeRowCount(int)), Qt::QueuedConnection);
    connect(mUi->table->horizontalHeader(), SIGNAL(sectionClicked(int)), this,
            SLOT(sortTable(int)), Qt::QueuedConnection);

    mRefreshMsg.trySend(REFRESH_START);
    mCarPropertyTableRefreshThread.start();
};

CarPropertyTable::~CarPropertyTable() {
    stopCarPropertyTableRefreshThread();
}

QString CarPropertyTable::getLabel(int row) {
    return mUi->table->item(row, LABEL_COLUMN)->text();
}

QString CarPropertyTable::getValueText(int row) {
    return mUi->table->item(row, VALUE_COLUMN)->text();
}

QString CarPropertyTable::getArea(int row) {
    return mUi->table->item(row, AREA_COLUMN)->text();
}

int CarPropertyTable::getPropertyId(int row) {
    return mUi->table->item(row, PROPERTY_ID_COLUMN)->text().toInt();
}

int CarPropertyTable::getAreaId(int row) {
    return mUi->table->item(row, AREA_ID_COLUMN)->text().toInt();
}

int CarPropertyTable::getType(int row) {
    return mUi->table->item(row, TYPE_COLUMN)->text().toInt();
}

// Wrapper slots to call the GUI-modifying functions from callback thread
void CarPropertyTable::sortTable(int column) {
    // Only allowing sorting on Label column, since, for example, sorting
    // by Values (which has different types/meanings) is nonsensical.
    if (column == LABEL_COLUMN) {
        mUi->table->sortItems(column, mNextOrder);
        updateIndex();
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

void CarPropertyTable::updateTable(int row, int col, QTableWidgetItem* item) {
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    mUi->table->setItem(row, col, item);
}

QTableWidgetItem* CarPropertyTable::createTableTextItem(QString info) {
    return new QTableWidgetItem(info);
}

void CarPropertyTable::updateIndex() {
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
                // Some received constants are vendor id and aren't on the list.
                // so if constants is vendor id, transfer raw value to hex and
                // build as vender label like Vendor(id: XXX) where XXX is hex
                // representation of the property. If not, show raw value.
                if (carpropertyutils::isVendor(val.prop())) {
                    propMap[val.prop()] = { carpropertyutils::vendorIdToString(val.prop()) };
                } else {
                    propMap[val.prop()] = { QString::number(val.prop()) };
                }
            }
            PropertyDescription propDesc = propMap[val.prop()];

            QString label = propDesc.label;
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
                emit updateData(tableIndex[key], VALUE_COLUMN, createTableTextItem(value));
            } else {
                emit updateData(valIndex, LABEL_COLUMN, createTableTextItem(label));
                emit updateData(valIndex, VALUE_COLUMN, createTableTextItem(value));
                emit updateData(valIndex, AREA_COLUMN, createTableTextItem(area));
                emit updateData(valIndex, PROPERTY_ID_COLUMN,
                                 createTableTextItem(QString::number(val.prop())));
                emit updateData(valIndex, AREA_ID_COLUMN,
                                 createTableTextItem(QString::number(val.area_id())));
                emit updateData(valIndex, TYPE_COLUMN,
                                 createTableTextItem(QString::number(val.value_type())));
            }
        }
        if (addedNewEntries) {
            mNextOrder = Qt::AscendingOrder;
            emit sort(0);
        }
    }
}

void CarPropertyTable::on_table_cellClicked(int row, int column) {
    int prop = getPropertyId(row);
    PropertyDescription propDesc = propMap[prop];
    if (column != VALUE_COLUMN || !propDesc.writable) {
        return;
    }
    int areaId = getAreaId(row);
    int type = getType(row);

    bool pressedOk;
    int32_t int32Value;
    float floatValue;

    EmulatorMessage writeMsg;
    string writeLog;
    switch (type) {
        // TODO: Account for the other types, although there aren't too many
        // more writable ones that fit well on the table.
        case (int32_t) VehiclePropertyType::BOOLEAN :
            // Booleans are actually sent as ints with values of 0/1.
            int32Value = getUserBoolValue(propDesc, row, &pressedOk);
            if (!pressedOk) {
                return;
            }
            writeMsg = makeSetPropMsgInt32(prop, int32Value, areaId);
            writeLog = "Setting value for " + getLabel(row).toStdString();
            mSendEmulatorMsg(writeMsg, writeLog);
            break;

        case (int32_t) VehiclePropertyType::INT32 :
            int32Value = getUserInt32Value(propDesc, row, &pressedOk);
            if (!pressedOk) {
                return;
            }
            writeMsg = makeSetPropMsgInt32(prop, int32Value, areaId);
            writeLog = "Setting value for " + getLabel(row).toStdString();
            mSendEmulatorMsg(writeMsg, writeLog);
            break;

        case (int32_t) VehiclePropertyType::FLOAT :
            floatValue = getUserFloatValue(propDesc, row, &pressedOk);
            if (!pressedOk) {
                return;
            }
            writeMsg = makeSetPropMsgFloat(prop, floatValue, areaId);
            writeLog = "Setting value for " + getLabel(row).toStdString();
            mSendEmulatorMsg(writeMsg, writeLog);
            break;
        case (int32_t) VehiclePropertyType::INT32_VEC :
            const std::vector<int32_t>* int32VecValue =
                    getUserInt32VecValue(propDesc, row, &pressedOk);
            if (!pressedOk) {
                return;
            }
            writeMsg = makeSetPropMsgInt32Vec(prop, int32VecValue, areaId);
            writeLog = "Setting value for " + getLabel(row).toStdString();
            mSendEmulatorMsg(writeMsg, writeLog);
            break;
    }

    mSendEmulatorMsg(writeMsg, writeLog);
    EmulatorMessage getMsg = makeGetPropMsg(prop, areaId);
    string getLog = "Sending get request for " + getLabel(row).toStdString();
    mSendEmulatorMsg(getMsg, getLog);
}

int32_t CarPropertyTable::getUserBoolValue(PropertyDescription propDesc, int row,
                                            bool* pressedOk) {
    QString oldValueString = getValueText(row);
    QStringList items;
    items << tr("True") << tr("False");
    QString item = QInputDialog::getItem(this, propDesc.label, nullptr,
                                          items, items.indexOf(oldValueString), false, pressedOk);
    return (item == "True") ? 1 : 0;
}

int32_t CarPropertyTable::getUserInt32Value(PropertyDescription propDesc, int row,
                                             bool* pressedOk) {
    QString oldValueString = getValueText(row);
    int32_t value;
    if (propDesc.lookupTable != nullptr) {
        QStringList items;
        for (const auto &detail : *(propDesc.lookupTable)) {
            items << detail.second;
        }
        QString item = QInputDialog::getItem(this, propDesc.label, nullptr, items,
                                              items.indexOf(oldValueString), false, pressedOk);
        for (const auto &detail : *(propDesc.lookupTable)) {
            if (item == detail.second) {
                value = detail.first;
                break;
            }
        }
    } else {
        int32_t oldValue = getValueText(row).toInt();
        value = QInputDialog::getInt(this, propDesc.label, nullptr, oldValue,
                                      INT_MIN, INT_MAX, 1, pressedOk);
    }
    return value;
}

const std::vector<int32_t>* CarPropertyTable::getUserInt32VecValue(
        carpropertyutils::PropertyDescription propDesc,
        int row,
        bool* pressedOk) {
    QString oldValueString = getValueText(row);
    QStringList valueStringList= oldValueString.split("; ");
    QSet<QString> oldStringSet = QSet<QString>::fromList(valueStringList);

    if (propDesc.lookupTable != nullptr) {
        CheckboxDialog checkboxDialog(this, propDesc.lookupTable, &oldStringSet, propDesc.label);
        if(checkboxDialog.exec() == QDialog::Accepted) {
            *pressedOk = true;
            return checkboxDialog.getVec();
        } else {
            *pressedOk = false;
        }
    }
    return nullptr;
}

float CarPropertyTable::getUserFloatValue(PropertyDescription propDesc, int row,
                                           bool* pressedOk) {
    // No property interprets floats with table, so we only deal with raw numbers.
    double oldValue = getValueText(row).toDouble();
    double value = QInputDialog::getDouble(this, propDesc.label, nullptr, oldValue,
                                            FLT_MIN, FLT_MAX, 3, pressedOk);
    return value;
}

EmulatorMessage CarPropertyTable::makeSetPropMsg(int propId, VehiclePropValue** valueRef,
                                                  int areaId) {
    EmulatorMessage emulatorMsg;
    emulatorMsg.set_msg_type(MsgType::SET_PROPERTY_CMD);
    emulatorMsg.set_status(Status::RESULT_OK);
    VehiclePropValue* value = emulatorMsg.add_value();
    value->set_prop(propId);
    value->set_area_id(areaId);
    (*valueRef) = value;
    return emulatorMsg;
}

EmulatorMessage CarPropertyTable::makeSetPropMsgInt32(int32_t propId, int val, int areaId) {
    VehiclePropValue* value;
    EmulatorMessage emulatorMsg = makeSetPropMsg(propId, &value, areaId);
    value->add_int32_values(val);
    return emulatorMsg;
}

EmulatorMessage CarPropertyTable::makeSetPropMsgFloat(int32_t propId, float val, int areaId) {
    VehiclePropValue* value;
    EmulatorMessage emulatorMsg = makeSetPropMsg(propId, &value, areaId);
    value->add_float_values(val);
    return emulatorMsg;
}

EmulatorMessage CarPropertyTable::makeSetPropMsgInt32Vec(
        int32_t propId,
        const std::vector<int32_t>* vals,
        int areaId) {
    VehiclePropValue* value;
    EmulatorMessage emulatorMsg = makeSetPropMsg(propId, &value, areaId);
    for(int32_t val : *vals){
        value->add_int32_values(val);
    }
    return emulatorMsg;
}

EmulatorMessage CarPropertyTable::makeGetPropMsg(int32_t prop, int areaId) {
    EmulatorMessage emulatorMsg;
    emulatorMsg.set_msg_type(MsgType::GET_PROPERTY_CMD);
    emulatorMsg.set_status(Status::RESULT_OK);
    VehiclePropGet* getMsg = emulatorMsg.add_prop();
    getMsg->set_prop(prop);
    getMsg->set_area_id(areaId);
    return emulatorMsg;
}

void CarPropertyTable::sendGetAllPropertiesRequest() {
    EmulatorMessage emulatorMsg;
    emulatorMsg.set_msg_type(MsgType::GET_PROPERTY_ALL_CMD);
    emulatorMsg.set_status(Status::RESULT_OK);
    string log = "Requesting all properties";
    mSendEmulatorMsg(emulatorMsg, log);
}

void CarPropertyTable::showEvent(QShowEvent*) {
    // start asking data
    setCarPropertyTableRefreshThread();
}

void CarPropertyTable::hideEvent(QHideEvent*) {
    // stop asking data
    pauseCarPropertyTableRefreshThread();
}

void CarPropertyTable::setSendEmulatorMsgCallback(CarSensorData::EmulatorMsgCallback&& func) {
    mSendEmulatorMsg = std::move(func);
}

android::base::System::Duration CarPropertyTable::nextRefreshAbsolute() {
    return android::base::System::get()->getUnixTimeUs() + REFRESH_INTERVEL;
}

void CarPropertyTable::setCarPropertyTableRefreshThread(){
    mRefreshMsg.trySend(REFRESH_START);
    mCarPropertyTableRefreshCV.signal();
}

void CarPropertyTable::stopCarPropertyTableRefreshThread(){
    mRefreshMsg.trySend(REFRESH_STOP);
    mCarPropertyTableRefreshCV.signal();
    mCarPropertyTableRefreshThread.wait();
}

void CarPropertyTable::pauseCarPropertyTableRefreshThread(){
    mRefreshMsg.trySend(REFRESH_PUASE);
}
