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
    struct PropWrapper {
        std::string label;
        std::map<int32_t, std::string>* lookupTable;
        std::string (*int32ToString)(int32_t val);
        std::string (*int32VecToString)(std::vector<int32_t> vals);
    };

    extern std::map<int32_t, PropWrapper> propMap;

    std::string booleanToString(PropWrapper prop, int32_t val);
    std::string int32ToString(PropWrapper prop, int32_t val);
    std::string int32VecToString(PropWrapper prop, std::vector<int32_t> vals);
    std::string int64ToString(PropWrapper prop, int64_t val);
    std::string int64VecToString(PropWrapper prop, std::vector<int64_t> vals);
    std::string floatToString(PropWrapper prop, float val);
    std::string floatVecToString(PropWrapper prop, std::vector<float> vals);

    std::string getValueString(emulator::VehiclePropValue val);
    std::string getAreaString(emulator::VehiclePropValue val);

    std::string multiDetailToString(std::map<int, std::string> lookupTable, int value);
    std::string fanDirectionToString(int32_t val);
    std::string heatingCoolingToString(int32_t val);
    std::string apPowerStateReqToString(std::vector<int32_t> vals);
    std::string apPowerStateReportToString(std::vector<int32_t> vals);
}  // namespace carpropertyutils

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
