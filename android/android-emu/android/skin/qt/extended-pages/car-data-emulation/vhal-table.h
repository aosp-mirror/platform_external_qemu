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
#pragma once

// TODO: (b/120444474) rename ERROR_INVALID_OPERATION & remove this undef
#undef ERROR_INVALID_OPERATION

#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"
#include "android/skin/qt/extended-pages/car-data-emulation/car-property-utils.h"
#include "android/skin/qt/extended-pages/car-data-emulation/car-sensor-data.h"

// TODO: (b/120444474) rename ERROR_INVALID_OPERATION & remove this undef
#undef ERROR_INVALID_OPERATION
#include <qobjectdefs.h>
#include <stdint.h>
#include <QString>
#include <QWidget>
#include <map>
#include <memory>
#include <vector>

#include "VehicleHalProto.pb.h"

class QHideEvent;
class QLabel;
class QListWidgetItem;
class QObject;
class QShowEvent;
class QString;
class QWidget;
class VhalItem;

#ifdef _MSC_VER
#include "msvc-posix.h"
#else
#endif

using android::base::FunctorThread;
using android::base::System;

namespace Ui {
class VhalTable;
}

// VhalTable is a component under emulator, extended page, car data.
// It's used to show vhal properties for Automotive OS
class VhalTable : public QWidget {
    Q_OBJECT

public:
    explicit VhalTable(QWidget* parent = nullptr);
    ~VhalTable();
    void processMsg(emulator::EmulatorMessage emulatorMsg);
    void setSendEmulatorMsgCallback(CarSensorData::EmulatorMsgCallback&&);

signals:
    void updateData(QString label,
                    QString propertyId,
                    QString areaId,
                    QString key);

private slots:
    void on_property_list_itemClicked(QListWidgetItem* item);
    void on_property_list_currentItemChanged(QListWidgetItem* current,
                                             QListWidgetItem* previous);
    void on_editButton_clicked();
    void updateTable(QString label,
                     QString propertyId,
                     QString areaId,
                     QString key);
    void refresh_filter(QString patern);

private:
    std::unique_ptr<Ui::VhalTable> mUi;

    // This map is to store the property get back from VHAL
    // Key is property name + areaID
    // Value is PropertyDescription
    std::map<QString, emulator::VehiclePropValue> mVHalpropertyMap;
    QString mSelectedKey;

    void initVhalPropertyTableRefreshThread();

    VhalItem* getItemWidget(QListWidgetItem* listItem);
    void setPropertyText(QLabel* label, QString text);

    void sendGetAllPropertiesRequest();
    CarSensorData::EmulatorMsgCallback mSendEmulatorMsg;
    emulator::EmulatorMessage makeGetPropMsg(int32_t prop, int areaId);

    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent*);
    void showPropertyDescription(emulator::VehiclePropValue val);
    void showEditableArea(emulator::VehiclePropValue val);
    void hide_all();
    QString getPropKey(emulator::VehiclePropValue val);

    // table refresh funcitons
    FunctorThread mVhalPropertyTableRefreshThread;
    android::base::MessageChannel<int, 2> mRefreshMsg;
    android::base::ConditionVariable mVhalPropertyTableRefreshCV;
    android::base::Lock mVhalPropertyTableRefreshLock;
    android::base::System::Duration nextRefreshAbsolute();
    void setVhalPropertyTableRefreshThread();
    void stopVhalPropertyTableRefreshThread();
    void pauseVhalPropertyTableRefreshThread();

    emulator::EmulatorMessage makeSetPropMsgInt32(int32_t propId, int val, int areaId);
    emulator::EmulatorMessage makeSetPropMsgFloat(int32_t propId, float val, int areaId);
    emulator::EmulatorMessage makeSetPropMsgInt32Vec(
            int32_t propId,
            const std::vector<int32_t>* val,
            int areaId);
    emulator::EmulatorMessage makeSetPropMsg(int propId, emulator::VehiclePropValue** valueRef,
                                                  int areaId);
    int32_t getUserBoolValue(carpropertyutils::PropertyDescription propDesc,
                              QString oldValueString, bool* pressedOk);
    float getUserFloatValue(carpropertyutils::PropertyDescription propDesc,
                             QString oldValueString, bool* pressedOk);
    int32_t getUserInt32Value(carpropertyutils::PropertyDescription propDesc,
                               QString oldValueString, bool* pressedOk);
    const std::vector<int32_t>* getUserInt32VecValue(
            carpropertyutils::PropertyDescription propDesc,
            QString oldValueString,
            bool* pressedOk);
};
