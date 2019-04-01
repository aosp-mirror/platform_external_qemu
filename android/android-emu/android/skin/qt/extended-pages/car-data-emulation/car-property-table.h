// Copyright (C) 2018 The Android Open Source Project
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

#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"
#include "ui_car-property-table.h"

// TODO: (b/120444474) rename ERROR_INVALID_OPERATION & remove this undef
#undef ERROR_INVALID_OPERATION
#include <QWidget>
#include "android/emulation/proto/VehicleHalProto.pb.h"

#include <string>
#include "android/skin/qt/extended-pages/car-data-emulation/car-sensor-data.h"
#include <map>
#include <vector>

#ifdef _MSC_VER
#include "msvc-posix.h"
#else
#include <sys/time.h>
#endif

using android::base::System;
using android::base::FunctorThread;

namespace carpropertyutils {
    struct PropertyDescription {
        QString label;
        bool writable;
        std::map<int32_t, QString>* lookupTable;
        QString (*int32ToString)(int32_t val);
        QString (*int32VecToString)(std::vector<int32_t> vals);
    };

    // Map from property ids to description for conversion to human readable form.
    extern std::map<int32_t, PropertyDescription> propMap;

    QString booleanToString(PropertyDescription prop, int32_t val);
    QString int32ToString(PropertyDescription prop, int32_t val);
    QString int32VecToString(PropertyDescription prop, std::vector<int32_t> vals);
    QString int64ToString(PropertyDescription prop, int64_t val);
    QString int64VecToString(PropertyDescription prop, std::vector<int64_t> vals);
    QString floatToString(PropertyDescription prop, float val);
    QString floatVecToString(PropertyDescription prop, std::vector<float> vals);
    QString int32ToHexString(int32_t val);
 
    QString getValueString(emulator::VehiclePropValue val);
    QString getAreaString(emulator::VehiclePropValue val);

    QString multiDetailToString(std::map<int, QString> lookupTable, int value);
    QString fanDirectionToString(int32_t val);
    QString heatingCoolingToString(int32_t val);
    QString apPowerStateReqToString(std::vector<int32_t> vals);
    QString apPowerStateReportToString(std::vector<int32_t> vals);
    QString vendorIdToString(int32_t val);
    bool isVendor(int32_t val);

}  // namespace carpropertyutils

// Table displaying properties of the emulated car & their values.
class CarPropertyTable : public QWidget {
    Q_OBJECT

public:
    explicit CarPropertyTable(QWidget* parent = nullptr);
    ~CarPropertyTable();
    void processMsg(emulator::EmulatorMessage emulatorMsg);
    void setSendEmulatorMsgCallback(CarSensorData::EmulatorMsgCallback&&);

signals:
    void updateData(int row, int col, QTableWidgetItem* info);
    void sort(int column);
    void setRowCount(int size);

protected:
    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent* event);

private slots:
    void updateTable(int row, int col, QTableWidgetItem* info);
    void sortTable(int column);
    void changeRowCount(int size);
    void on_table_cellClicked(int row, int column);

private:
    std::unique_ptr<Ui::CarPropertyTable> mUi;
    CarSensorData::EmulatorMsgCallback mSendEmulatorMsg;
    Qt::SortOrder mNextOrder;

    QString getLabel(int row);
    QString getValueText(int row);
    QString getArea(int row);
    int getPropertyId(int row);
    int getAreaId(int row);
    int getType(int row);

    FunctorThread mCarPropertyTableRefreshThread;
    android::base::MessageChannel<int, 2> mRefreshMsg;
    android::base::ConditionVariable mCarPropertyTableRefreshCV;
    android::base::Lock mCarPropertyTableRefreshLock;
    android::base::System::Duration nextRefreshAbsolute();
    void setCarPropertyTableRefreshThread();
    void stopCarPropertyTableRefreshThread();
    void pauseCarPropertyTableRefreshThread();

    QTableWidgetItem* createTableTextItem(QString info);

    void updateIndex();

    void sendGetAllPropertiesRequest();

    emulator::EmulatorMessage makeGetPropMsg(int32_t prop, int areaId);
    emulator::EmulatorMessage makeSetPropMsg(int propId, emulator::VehiclePropValue** valueRef,
                                              int areaId);
    emulator::EmulatorMessage makeSetPropMsgInt32(int32_t propId, int val, int areaId);
    emulator::EmulatorMessage makeSetPropMsgFloat(int32_t propId, float val, int areaId);
    emulator::EmulatorMessage makeSetPropMsgInt32Vec(
            int32_t propId,
            const std::vector<int32_t>* val,
            int areaId);

    int32_t getUserBoolValue(carpropertyutils::PropertyDescription propDesc,
                              int row, bool* pressedOk);
    float getUserFloatValue(carpropertyutils::PropertyDescription propDesc,
                             int row, bool* pressedOk);
    int32_t getUserInt32Value(carpropertyutils::PropertyDescription propDesc,
                               int row, bool* pressedOk);
    const std::vector<int32_t>* getUserInt32VecValue(
            carpropertyutils::PropertyDescription propDesc,
            int row,
            bool* pressedOk);
};
