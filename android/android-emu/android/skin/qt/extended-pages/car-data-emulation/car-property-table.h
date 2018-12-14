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

#include "ui_car-property-table.h"

// TODO: (b/120444474) rename ERROR_INVALID_OPERATION & remove this undef
#undef ERROR_INVALID_OPERATION
#include "android/emulation/proto/VehicleHalProto.pb.h"
#include <QWidget>
#include <string>
#include "android/skin/qt/extended-pages/car-data-emulation/car-sensor-data.h"
#include <map>
#include <vector>

namespace carpropertyutils {
    struct PropertyDescription {
        QString label;
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

    QString getValueString(emulator::VehiclePropValue val);
    QString getAreaString(emulator::VehiclePropValue val);

    QString multiDetailToString(std::map<int, QString> lookupTable, int value);
    QString fanDirectionToString(int32_t val);
    QString heatingCoolingToString(int32_t val);
    QString apPowerStateReqToString(std::vector<int32_t> vals);
    QString apPowerStateReportToString(std::vector<int32_t> vals);
}  // namespace carpropertyutils

// Table displaying properties of the emulated car & their values.
class CarPropertyTable : public QWidget {
    Q_OBJECT

public:
    explicit CarPropertyTable(QWidget* parent = nullptr);
    void processMsg(emulator::EmulatorMessage emulatorMsg);
    void setSendEmulatorMsgCallback(CarSensorData::EmulatorMsgCallback&&);

signals:
    void updateData(int row, int col, QString info);
    void sort(int column);
    void setRowCount(int size);

protected:
    void showEvent(QShowEvent* event);

private slots:
    void updateTable(int row, int col, QString info);
    void sortTable(int column);
    void changeRowCount(int size);

private:
    std::unique_ptr<Ui::CarPropertyTable> mUi;
    CarSensorData::EmulatorMsgCallback mSendEmulatorMsg;
    Qt::SortOrder mNextOrder;
    void updateIndices();
    void sendGetAllPropertiesRequest();
};
