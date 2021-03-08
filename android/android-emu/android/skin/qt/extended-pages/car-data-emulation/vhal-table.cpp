// Copyright (C) 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "vhal-table.h"

#include <limits.h>
#include <qdialog.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qstring.h>
#include <QDialog>
#include <QFontMetrics>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSet>
#include <QStringList>
#include <cfloat>
#include <cstdint>
#include <functional>
#include <new>
#include <string>
#include <utility>

#include "VehicleHalProto.pb.h"
#include "android/skin/qt/extended-pages/car-data-emulation/checkbox-dialog.h"
#include "android/skin/qt/extended-pages/car-data-emulation/vehicle_constants_generated.h"
#include "android/skin/qt/raised-material-button.h"
#include "ui_vhal-table.h"
#include "vhal-item.h"

class QHideEvent;
class QLabel;
class QListWidgetItem;
class QShowEvent;
class QWidget;

using std::map;
using std::string;
using std::vector;

using emulator::EmulatorMessage;
using emulator::MsgType;
using emulator::Status;
using emulator::VehiclePropertyType;
using emulator::VehiclePropGet;
using emulator::VehiclePropValue;

using carpropertyutils::PropertyDescription;
using carpropertyutils::propMap;

static constexpr int REFRESH_START = 1;
static constexpr int REFRESH_STOP = 2;
static constexpr int REFRESH_PAUSE = 3;

static constexpr int64_t REFRESH_INTERVAL_MILLISECONDS = 1000000LL;

VhalTable::VhalTable(QWidget* parent)
    : QWidget(parent),
      mUi(new Ui::VhalTable),
      mVhalPropertyTableRefreshThread([this] {
          initVhalPropertyTableRefreshThread();
      }) {
    mUi->setupUi(this);

    connect(this, SIGNAL(updateData(QString, QString, QString, QString)), this,
            SLOT(updateTable(QString, QString, QString, QString)),
            Qt::QueuedConnection);
    connect(mUi->property_search, SIGNAL(textEdited(QString)), this,
            SLOT(refresh_filter(QString)));

    // start refresh thread
    mRefreshMsg.trySend(REFRESH_START);
    mVhalPropertyTableRefreshThread.start();
}

VhalTable::~VhalTable() {
    stopVhalPropertyTableRefreshThread();
}

void VhalTable::initVhalPropertyTableRefreshThread() {
    int msg;
    while (true) {
        mRefreshMsg.tryReceive(&msg);
        if (msg == REFRESH_STOP) {
            break;
        }
        android::base::AutoLock lock(mVhalPropertyTableRefreshLock);
        switch (msg) {
            case REFRESH_START:
                if (mSendEmulatorMsg != nullptr) {
                    sendGetAllPropertiesRequest();
                }
                mVhalPropertyTableRefreshCV.timedWait(
                        &mVhalPropertyTableRefreshLock,
                        nextRefreshAbsolute());
                break;
            case REFRESH_PAUSE:
                mVhalPropertyTableRefreshCV.wait(
                        &mVhalPropertyTableRefreshLock);
                break;
        }
    }
}

// Check data for selected item
void VhalTable::on_property_list_itemClicked(QListWidgetItem* item) {
    VhalItem* vhalItem = getItemWidget(item);
    int prop = vhalItem->getPropertyId();
    int areaId = vhalItem->getAreaId();
    QString key = vhalItem->getKey();

    mSelectedKey = key;

    EmulatorMessage getMsg = makeGetPropMsg(prop, areaId);
    string getLog =
            "Sending get request for " + QString::number(prop).toStdString();
    mSendEmulatorMsg(getMsg, getLog);
}

VhalItem* VhalTable::getItemWidget(QListWidgetItem* listItem) {
    return qobject_cast<VhalItem*>(mUi->property_list->itemWidget(listItem));
}

void VhalTable::on_property_list_currentItemChanged(QListWidgetItem* current,
                                                    QListWidgetItem* previous) {
    if (current && previous) {
        VhalItem* vhalItem = getItemWidget(previous);
        vhalItem->vhalItemSelected(false);
    }
    if (current) {
        VhalItem* vhalItem = getItemWidget(current);
        vhalItem->vhalItemSelected(true);
    }
}

void VhalTable::on_editButton_clicked() {
    if(mVHalpropertyMap.count(mSelectedKey)){
        showEditableArea(mVHalpropertyMap[mSelectedKey]);
    }
}

void VhalTable::sendGetAllPropertiesRequest() {
    EmulatorMessage emulatorMsg;
    emulatorMsg.set_msg_type(MsgType::GET_PROPERTY_ALL_CMD);
    emulatorMsg.set_status(Status::RESULT_OK);
    mSendEmulatorMsg(emulatorMsg, "Requesting all properties");
}

EmulatorMessage VhalTable::makeGetPropMsg(int32_t prop, int areaId) {
    EmulatorMessage emulatorMsg;
    emulatorMsg.set_msg_type(MsgType::GET_PROPERTY_CMD);
    emulatorMsg.set_status(Status::RESULT_OK);
    VehiclePropGet* getMsg = emulatorMsg.add_prop();
    getMsg->set_prop(prop);
    getMsg->set_area_id(areaId);
    return emulatorMsg;
}

void VhalTable::setSendEmulatorMsgCallback(
        CarSensorData::EmulatorMsgCallback&& func) {
    mSendEmulatorMsg = std::move(func);
}

void VhalTable::showEvent(QShowEvent* event) {
    // clear old property list and map
    // ask for new data.
    mUi->property_list->clear();
    mVHalpropertyMap.clear();
    mSelectedKey = QString();
    sendGetAllPropertiesRequest();
    setVhalPropertyTableRefreshThread();
}

void VhalTable::hideEvent(QHideEvent*) {
    // stop asking data
    pauseVhalPropertyTableRefreshThread();
}

void VhalTable::updateTable(QString label,
                            QString propertyId,
                            QString areaId,
                            QString key) {
    QListWidgetItem* item = new QListWidgetItem();
    mUi->property_list->addItem(item);
    VhalItem* ci = new VhalItem(nullptr, label,
                                QString::fromStdString("ID : ") + propertyId);
    ci->setValues(propertyId.toInt(), areaId.toInt(), key);
    item->setSizeHint(ci->size());
    mUi->property_list->setItemWidget(item, ci);
}

void VhalTable::processMsg(EmulatorMessage emulatorMsg) {
    if (emulatorMsg.prop_size() > 0 || emulatorMsg.value_size() > 0) {
        QStringList sl;
        for (int valIndex = 0; valIndex < emulatorMsg.value_size();
             valIndex++) {
            VehiclePropValue val = emulatorMsg.value(valIndex);

            QString key = getPropKey(val);

            // if the return value contains new property
            // like new sensors start during runtime
            if (!mVHalpropertyMap.count(key)) {
                sl << key;
            }
            mVHalpropertyMap[key] = val;
            // if the return val is the selected property
            // update the description board
            if (QString::compare(key, mSelectedKey, Qt::CaseSensitive) == 0) {
                showPropertyDescription(val);
            }
        }

        // Sort the keys and emit the output based on keys
        // only delta property will be rendered here
        sl.sort();
        for (int i = 0; i < sl.size(); i++) {
            QString key = sl.at(i);
            VehiclePropValue currVal = mVHalpropertyMap[key];
            PropertyDescription currPropDesc = propMap[currVal.prop()];
            QString label = currPropDesc.label;
            QString id = QString::number(currVal.prop());
            QString areaId = QString::number(currVal.area_id());

            emit updateData(label, id, areaId, key);
        }

        // set mSelectedKey to the first key if mSelectedKey is empty
        // This should only happen at the first time table is opened
        if (mSelectedKey.isEmpty() && sl.size() > 0) {
            mSelectedKey = sl.at(0);
            showPropertyDescription(mVHalpropertyMap[mSelectedKey]);
        }
    }
}

QString VhalTable::getPropKey(VehiclePropValue val) {
    if (!propMap.count(val.prop())) {
        // Some received constants are vendor id and aren't on the list.
        // so if constants is vendor id, transfer raw value to hex and
        // build as vender label like Vendor(id: XXX) where XXX is hex
        // representation of the property. If not, show raw value.
        if (carpropertyutils::isVendor(val.prop())) {
            propMap[val.prop()] = {
                    carpropertyutils::vendorIdToString(val.prop())};
        } else {
            propMap[val.prop()] = {QString::number(val.prop())};
        }
    }

    PropertyDescription propDesc = propMap[val.prop()];

    QString label = propDesc.label;
    QString area = carpropertyutils::getAreaString(val);
    return label + area;
}

void VhalTable::showPropertyDescription(VehiclePropValue val) {
    PropertyDescription propDesc = propMap[val.prop()];
    QString label = propDesc.label;
    QString area = carpropertyutils::getAreaString(val);
    QString id = QString::number(val.prop());
    QString value = carpropertyutils::getValueString(val);
    int type = val.value_type();

    setPropertyText(mUi->property_name_value, label);
    setPropertyText(mUi->area_value, area);
    setPropertyText(mUi->property_id_value, id);
    setPropertyText(mUi->editable_value,
                    propDesc.writable ? QString::fromStdString("Yes")
                                      : QString::fromStdString("No"));
    setPropertyText(mUi->value_value, value);
    mUi->editButton->setEnabled(propDesc.writable);
}

void VhalTable::setPropertyText(QLabel* label, QString text) {
    QFontMetrics metrix(label->font());
    int width = label->width() - 2;
    QString clippedText = metrix.elidedText(text, Qt::ElideRight, width);
    label->setText(clippedText);
}

void VhalTable::showEditableArea(VehiclePropValue val) {
    int prop = val.prop();
    int type = val.value_type();
    int areaId = val.area_id();
    PropertyDescription propDesc = propMap[prop];
    QString value = carpropertyutils::getValueString(val);
    QString label = propDesc.label;

    bool pressedOk;
    int32_t int32Value;
    float floatValue;

    EmulatorMessage writeMsg;
    string writeLog;

    switch (type) {
        case (int32_t)VehiclePropertyType::BOOLEAN:
            int32Value = getUserBoolValue(propDesc, value, &pressedOk);
            if (!pressedOk) {
                return;
            }
            writeMsg = makeSetPropMsgInt32(prop, int32Value, areaId);
            writeLog = "Setting value for " + label.toStdString();
            mSendEmulatorMsg(writeMsg, writeLog);
            break;

        case (int32_t)VehiclePropertyType::INT32:
            int32Value = getUserInt32Value(propDesc, value, &pressedOk);
            if (!pressedOk) {
                return;
            }
            writeMsg = makeSetPropMsgInt32(prop, int32Value, areaId);
            writeLog = "Setting value for " + label.toStdString();
            mSendEmulatorMsg(writeMsg, writeLog);
            break;

        case (int32_t)VehiclePropertyType::FLOAT:
            floatValue = getUserFloatValue(propDesc, value, &pressedOk);
            if (!pressedOk) {
                return;
            }
            writeMsg = makeSetPropMsgFloat(prop, floatValue, areaId);
            writeLog = "Setting value for " + label.toStdString();
            mSendEmulatorMsg(writeMsg, writeLog);
            break;
        case (int32_t)VehiclePropertyType::INT32_VEC:
            const std::vector<int32_t>* int32VecValue =
                    getUserInt32VecValue(propDesc, value, &pressedOk);
            if (!pressedOk) {
                return;
            }
            writeMsg = makeSetPropMsgInt32Vec(prop, int32VecValue, areaId);
            writeLog = "Setting value for " + label.toStdString();
            mSendEmulatorMsg(writeMsg, writeLog);
            break;
    }
}

int32_t VhalTable::getUserBoolValue(PropertyDescription propDesc, QString oldValueString,
                                            bool* pressedOk) {
    QStringList items;
    items << tr("True") << tr("False");
    QString item = QInputDialog::getItem(this, propDesc.label, nullptr,
                                          items, items.indexOf(oldValueString), false, pressedOk);
    return (item == "True") ? 1 : 0;
}

int32_t VhalTable::getUserInt32Value(PropertyDescription propDesc, QString oldValueString,
                                             bool* pressedOk) {
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
        int32_t oldValue = oldValueString.toInt();
        value = QInputDialog::getInt(this, propDesc.label, nullptr, oldValue,
                                      INT_MIN, INT_MAX, 1, pressedOk);
    }
    return value;
}

const std::vector<int32_t>* VhalTable::getUserInt32VecValue(
        carpropertyutils::PropertyDescription propDesc,
        QString oldValueString,
        bool* pressedOk) {
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

float VhalTable::getUserFloatValue(PropertyDescription propDesc, QString oldValueString,
                                           bool* pressedOk) {
    // No property interprets floats with table, so we only deal with raw numbers.
    double oldValue = oldValueString.toDouble();
    double value = QInputDialog::getDouble(this, propDesc.label, nullptr, oldValue,
                                            FLT_MIN, FLT_MAX, 3, pressedOk);
    return value;
}

EmulatorMessage VhalTable::makeSetPropMsgInt32(int32_t propId, int val, int areaId) {
    VehiclePropValue* value;
    EmulatorMessage emulatorMsg = makeSetPropMsg(propId, &value, areaId);
    value->add_int32_values(val);
    return emulatorMsg;
}

EmulatorMessage VhalTable::makeSetPropMsgFloat(int32_t propId, float val, int areaId) {
    VehiclePropValue* value;
    EmulatorMessage emulatorMsg = makeSetPropMsg(propId, &value, areaId);
    value->add_float_values(val);
    return emulatorMsg;
}

EmulatorMessage VhalTable::makeSetPropMsgInt32Vec(
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


EmulatorMessage VhalTable::makeSetPropMsg(int propId, VehiclePropValue** valueRef,
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


void VhalTable::refresh_filter(QString patern) {
    hide_all();
    for (int row(0); row < mUi->property_list->count(); row++) {
        QListWidgetItem* item = mUi->property_list->item(row);
        VhalItem* vhalItem = getItemWidget(item);
        QString key = vhalItem->getKey();

        if (key.contains(patern, Qt::CaseInsensitive)) {
            item->setHidden(false);
        }
    }
}

void VhalTable::hide_all() {
    for (int row(0); row < mUi->property_list->count(); row++)
        mUi->property_list->item(row)->setHidden(true);
}

// Ask property update every 1s
android::base::System::Duration VhalTable::nextRefreshAbsolute() {
    return android::base::System::get()->getUnixTimeUs() + REFRESH_INTERVAL_MILLISECONDS;
}

void VhalTable::setVhalPropertyTableRefreshThread() {
    mRefreshMsg.trySend(REFRESH_START);
    mVhalPropertyTableRefreshCV.signal();
}

void VhalTable::stopVhalPropertyTableRefreshThread() {
    mRefreshMsg.trySend(REFRESH_STOP);
    mVhalPropertyTableRefreshCV.signal();
    mVhalPropertyTableRefreshThread.wait();
}

void VhalTable::pauseVhalPropertyTableRefreshThread() {
    mRefreshMsg.trySend(REFRESH_PAUSE);
}
