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
#include <QDir>
#include <QDialog>
#include <QFontMetrics>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSet>
#include <QString>
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
#include "aemu/base/Log.h"                               // for LogStream...
#include "android/avd/info.h"
#include "android/avd/util.h"                         // for AVD_ANDROID_AUTO
#include "aemu/base/files/PathUtils.h"
#include "android/console.h"  // for getConsoleAgents, AndroidCons...

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
using carpropertyutils::changeModeToString;
using carpropertyutils::loadDescriptionsFromJson;

static constexpr int REFRESH_START = 1;
static constexpr int REFRESH_STOP = 2;
static constexpr int REFRESH_PAUSE = 3;

static constexpr int64_t REFRESH_INTERVAL_USECONDS = 1000000LL;

#include "android/utils/debug.h"

#define D(...) VERBOSE_PRINT(car, __VA_ARGS__)

VhalTable::VhalTable(QWidget* parent)
    : QWidget(parent),
      mUi(new Ui::VhalTable),
      mVhalPropertyTableRefreshThread([this] {
          initVhalPropertyTableRefreshThread();
      }) {

    initVhalPropertyTable();

    mUi->setupUi(this);

    connect(this, SIGNAL(updateNewData(QStringList)), this,
            SLOT(updateTable(QStringList)), Qt::QueuedConnection);
    connect(this, SIGNAL(updateData(QStringList)), this,
            SLOT(updateProperties(QStringList)), Qt::QueuedConnection);
    connect(mUi->property_search, SIGNAL(textEdited(QString)), this,
            SLOT(refresh_filter(QString)));

    // start refresh thread
    mRefreshMsg.trySend(REFRESH_START);
    mVhalPropertyTableRefreshThread.start();
}

VhalTable::~VhalTable() {
    stopVhalPropertyTableRefreshThread();
}

void VhalTable::initVhalPropertyTable() {
    if (avdInfo_getAvdFlavor(getConsoleAgents()->settings->avdInfo()) != AVD_ANDROID_AUTO) {
        // VHAL is only relevant to automotive devices
        return;
    }
    // Extend VHAL properties dictionaries with json descriptions
    QFileInfo sysImgFileInfo(avdInfo_getSystemImagePath(getConsoleAgents()->settings->avdInfo()));
    QString sysImgPath(sysImgFileInfo.absolutePath());

    QString avdPath(avdInfo_getContentPath(getConsoleAgents()->settings->avdInfo()));
    QStringList paths = {sysImgPath, avdPath};
    foreach(QString path, paths) {
        // Search for vhal files in avd and system image path
        if (!path.isEmpty()) {
            QDir pathDir(path);
            QStringList metaFiles = pathDir.entryList(QStringList() << "*types-meta.json", QDir::Files);
            foreach(QString filename, metaFiles) {
                loadDescriptionsFromJson(filename.prepend("/").prepend(path).toStdString().c_str());
                return;
            }
        }
    }
    LOG(INFO) << "Did not find a vhal json" << std::endl;
}

void VhalTable::initVhalPropertyTableRefreshThread() {
    int msg;
    while (true) {
        // Receive only the last message (bug :210075881)
        while(mRefreshMsg.tryReceive(&msg));
        if (msg == REFRESH_STOP) {
            break;
        }
        android::base::AutoLock lock(mVhalPropertyTableRefreshLock);
        switch (msg) {
            case REFRESH_START:
                if (mSendEmulatorMsg != nullptr) {
                    sendGetSelectedPropertyRequest();
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

void VhalTable::sendGetSelectedPropertyRequest() {
    if (mSelectedKey.isEmpty()) {
        return;
    }

    if (!mVHalPropValuesMap.count(mSelectedKey)) {
        return;
    }

    VehiclePropValue currVal = mVHalPropValuesMap[mSelectedKey];

    EmulatorMessage getMsg = makeGetPropMsg(currVal.prop(), currVal.area_id());
    string getLog = "Sending get request for " +
                    QString::number(currVal.prop()).toStdString();
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
        mSelectedKey = vhalItem->getKey();
        clearPropertyDescription();
        sendGetSelectedPropertyRequest();
    } else {
        mSelectedKey = QString();
        clearPropertyDescription();
    }
}

void VhalTable::on_editButton_clicked() {
    if (mSelectedKey.isEmpty()) {
        return;
    }
    if (mVHalPropValuesMap.count(mSelectedKey)) {
        showEditableArea(mVHalPropValuesMap[mSelectedKey]);
    }
}

void VhalTable::sendGetAllPropertiesRequest() {
    EmulatorMessage getAllConfigsMsg;
    getAllConfigsMsg.set_msg_type(MsgType::GET_CONFIG_ALL_CMD);
    getAllConfigsMsg.set_status(Status::RESULT_OK);
    mSendEmulatorMsg(getAllConfigsMsg, "Requesting all configs");

    EmulatorMessage emulatorMsg;
    emulatorMsg.set_msg_type(MsgType::GET_PROPERTY_ALL_CMD);
    emulatorMsg.set_status(Status::RESULT_OK);
    mSendEmulatorMsg(emulatorMsg, "Requesting all values");
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
    mVHalPropValuesMap.clear();
    mSelectedKey = QString();
    clearPropertyDescription();
    sendGetAllPropertiesRequest();
    setVhalPropertyTableRefreshThread();
}

void VhalTable::hideEvent(QHideEvent*) {
    // stop asking data
    pauseVhalPropertyTableRefreshThread();
}

void VhalTable::updateTable(QStringList sl) {
    int current_size = mUi->property_list->count();

    mUi->property_list->addItems(sl);

    for (int i = 0; i < sl.size(); i++) {
        QString key = sl.at(i);
        VehiclePropValue currVal = mVHalPropValuesMap[key];
        PropertyDescription currPropDesc = propMap[currVal.prop()];
        QString label = currPropDesc.label;
        QString id = QString::number(currVal.prop());
        QString areaId = QString::number(currVal.area_id());
        QListWidgetItem* item = mUi->property_list->item(current_size+i);
        item->setText(nullptr);
        VhalItem* ci = new VhalItem(nullptr, label,
                                QString::fromStdString("ID : ") + id);
        ci->setValues(id.toInt(), areaId.toInt(), key);
        item->setSizeHint(ci->size());
        mUi->property_list->setItemWidget(item, ci);
  }

    // select first item if mSelectedKey is empty
    // This should only happen at the first time table is opened
    if (mSelectedKey.isEmpty() && sl.size() > 0) {
        mUi->property_list->setCurrentRow(0);
    }

  D("updateTable() complete");
}

void VhalTable::updateProperties(QStringList sl) {
    if (sl.contains(mSelectedKey)) {
        VehiclePropValue currVal = mVHalPropValuesMap[mSelectedKey];
        showPropertyDescription(currVal);
    }
}

void VhalTable::processMsg(EmulatorMessage emulatorMsg) {
    switch (emulatorMsg.msg_type()) {
        case (int32_t)MsgType::GET_PROPERTY_RESP:
        case (int32_t)MsgType::GET_PROPERTY_ALL_RESP:
            if (emulatorMsg.value_size() > 0) {
                QStringList newKeys;
                QStringList updatedKeys;
                for (int valIndex = 0; valIndex < emulatorMsg.value_size();
                    valIndex++) {
                    VehiclePropValue val = emulatorMsg.value(valIndex);
                    QString key = getPropKey(val);
                    // if the return value contains new property
                    // like new sensors start during runtime
                    if (!mVHalPropValuesMap.count(key)) {
                        newKeys << key;
                    }
                    mVHalPropValuesMap[key] = val;
                    updatedKeys << key;
                }

                if (newKeys.size() > 0) {
                    // Sort the keys and emit the output based on keys
                    // only delta properties will be emitted
                    newKeys.sort();
                    emit updateNewData(newKeys);
                }
                if (updatedKeys.size() > 0) {
                    emit updateData(updatedKeys);
                }
            }
            break;
        case (int32_t)MsgType::GET_CONFIG_ALL_RESP:
            for (int configIndex = 0; configIndex < emulatorMsg.config_size();
                    configIndex++) {
                emulator::VehiclePropConfig config =
                                              emulatorMsg.config(configIndex);
                mVHalPropConfigMap[config.prop()] = config;
            }
            break;
        default:
            // Unexpected message type, ignore it
            break;
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
    emulator::VehiclePropConfig propConfig = mVHalPropConfigMap[val.prop()];
    QString label = propDesc.label;
    QString area = carpropertyutils::getAreaString(val);
    QString id = QString::number(val.prop());
    QString value = carpropertyutils::getValueString(val);
    int type = val.value_type();

    mUi->property_name_value->setText(label);
    mUi->area_value->setText(area);
    mUi->property_id_value->setText(id);
    mUi->change_mode_value->setText(changeModeToString(propConfig.change_mode()));
    mUi->value_value->setText(value);
    auto description = carpropertyutils::getDetailedDescription(val.prop());
    if (mUi->property_description_value->toPlainText().isEmpty() && !description.isEmpty()) {
        // Don't overwrite the description if already set to prevent resetting the scroll position.
        mUi->property_description_value->setText(description);
    }
    mUi->editButton->setEnabled(propConfig.access() != emulator::VehiclePropertyAccess::WRITE);
}

void VhalTable::clearPropertyDescription() {
    mUi->property_name_value->setText(QString());
    mUi->area_value->setText(QString());
    mUi->property_id_value->setText(QString());
    mUi->change_mode_value->setText(QString());
    mUi->value_value->setText(QString());
    mUi->property_description_value->setText(QString());
}

void VhalTable::showEditableArea(VehiclePropValue val) {
    static const QString editingStaticWarning =
        tr("WARNING: static properties cannot be subscribed to,\n"
           "so clients need a get() call to fetch an updated value.\n"
           "This can be achieved, e.g. by restarting the client.");

    int prop = val.prop();
    int type = val.value_type();
    int areaId = val.area_id();
    PropertyDescription propDesc = propMap[prop];
    emulator::VehiclePropConfig propConfig = mVHalPropConfigMap[val.prop()];
    QString value = carpropertyutils::getValueString(val);
    QString label = propDesc.label;
    QString tip = nullptr;
    if (propConfig.change_mode() == (int32_t)emulator::VehiclePropertyChangeMode::STATIC) {
        tip = editingStaticWarning;
    }

    bool pressedOk;
    int32_t int32Value;
    float floatValue;
    std::string stringValue;

    EmulatorMessage writeMsg;
    string writeLog;

    switch (type) {
        case (int32_t)VehiclePropertyType::BOOLEAN:
            int32Value = getUserBoolValue(propDesc, value, tip, &pressedOk);
            if (!pressedOk) {
                return;
            }
            writeMsg = makeSetPropMsgInt32(prop, int32Value, areaId);
            writeLog = "Setting value for " + label.toStdString();
            mSendEmulatorMsg(writeMsg, writeLog);
            break;

        case (int32_t)VehiclePropertyType::INT32:
            int32Value = getUserInt32Value(propDesc, value, tip, &pressedOk);
            if (!pressedOk) {
                return;
            }
            writeMsg = makeSetPropMsgInt32(prop, int32Value, areaId);
            writeLog = "Setting value for " + label.toStdString();
            mSendEmulatorMsg(writeMsg, writeLog);
            break;

        case (int32_t)VehiclePropertyType::FLOAT:
            floatValue = getUserFloatValue(propDesc, value, tip, &pressedOk);
            if (!pressedOk) {
                return;
            }
            writeMsg = makeSetPropMsgFloat(prop, floatValue, areaId);
            writeLog = "Setting value for " + label.toStdString();
            mSendEmulatorMsg(writeMsg, writeLog);
            break;

        case (int32_t)VehiclePropertyType::STRING:
            stringValue = getUserStringValue(propDesc, value, tip, &pressedOk).toStdString();
            if (!pressedOk) {
                return;
            }
            writeMsg = makeSetPropMsgString(prop, stringValue, areaId);
            writeLog = "Setting string value for " + label.toStdString();
            mSendEmulatorMsg(writeMsg, writeLog);
            break;

        case (int32_t)VehiclePropertyType::INT32_VEC:
            const std::vector<int32_t>* int32VecValue =
                    getUserInt32VecValue(propDesc, value, tip, &pressedOk);
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
                                            QString tip, bool* pressedOk) {
    QStringList items;
    items << tr("True") << tr("False");
    QString item = QInputDialog::getItem(this, propDesc.label, tip,
                                          items, items.indexOf(oldValueString), false, pressedOk);
    return (item == "True") ? 1 : 0;
}

int32_t VhalTable::getUserInt32Value(PropertyDescription propDesc, QString oldValueString,
                                            QString tip, bool* pressedOk) {
    int32_t value;
    QStringList items;
    map<QString, int32_t> valueByName;
    for (int i = 0; i < propDesc.lookupTableNames.size(); i++) {
        const auto& lookupTableName = propDesc.lookupTableNames[i];
        auto lookupTable = carpropertyutils::lookupTablesMap.find(lookupTableName);
        if (lookupTable == carpropertyutils::lookupTablesMap.end()) {
            continue;
        }

        for (const auto &detail : *(lookupTable->second)) {
            items << detail.second;
            valueByName[detail.second] = detail.first;
        }
    } 
    if (items.size() != 0) {
        QString item = QInputDialog::getItem(this, propDesc.label, tip, items,
                                              items.indexOf(oldValueString), false, pressedOk);
        return valueByName[item];
    }
    int32_t oldValue = oldValueString.toInt();
    value = QInputDialog::getInt(this, propDesc.label, tip, oldValue,
                                  INT_MIN, INT_MAX, 1, pressedOk);
    return value;
}

const std::vector<int32_t>* VhalTable::getUserInt32VecValue(
        carpropertyutils::PropertyDescription propDesc,
        QString oldValueString,
        QString tip,
        bool* pressedOk) {
    QStringList valueStringList = oldValueString.split("; ");
    QSet<QString> oldStringSet(valueStringList.constBegin(), valueStringList.constEnd());

    map<int32_t, QString> nameByValue;
    for (int i = 0; i < propDesc.lookupTableNames.size(); i++) {
        const auto& lookupTableName = propDesc.lookupTableNames[i];
        auto lookupTable = carpropertyutils::lookupTablesMap.find(lookupTableName);
        if (lookupTable == carpropertyutils::lookupTablesMap.end()) {
            continue;
        }
        for (const auto& [value, name] : *lookupTable->second) {
            nameByValue[value] = name;
        }
    }

    if (nameByValue.size() != 0) {
        CheckboxDialog checkboxDialog(this, &nameByValue, &oldStringSet, propDesc.label, tip);
        if(checkboxDialog.exec() == QDialog::Accepted) {
            *pressedOk = true;
            return checkboxDialog.getVec();
        }
    }
    *pressedOk = false;
    return nullptr;
}

float VhalTable::getUserFloatValue(PropertyDescription propDesc, QString oldValueString,
                                           QString tip, bool* pressedOk) {
    // No property interprets floats with table, so we only deal with raw numbers.
    double oldValue = oldValueString.toDouble();
    double value = QInputDialog::getDouble(this, propDesc.label, tip, oldValue,
                                            FLT_MIN, FLT_MAX, 3, pressedOk);
    return value;
}

QString VhalTable::getUserStringValue(PropertyDescription propDesc, QString oldValueString,
                                           QString tip, bool* pressedOk) {
    QString value = QInputDialog::getText(this, propDesc.label, tip,
                                            QLineEdit::EchoMode::Normal, oldValueString,
                                            pressedOk);
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

EmulatorMessage VhalTable::makeSetPropMsgString(int32_t propId, const std::string val, int areaId) {
    VehiclePropValue* value;
    EmulatorMessage emulatorMsg = makeSetPropMsg(propId, &value, areaId);
    value->set_string_value(val);
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
    return android::base::System::get()->getUnixTimeUs() + REFRESH_INTERVAL_USECONDS;
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
