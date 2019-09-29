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

// TODO: (b/120444474) rename ERROR_INVALID_OPERATION & remove this undef
#undef ERROR_INVALID_OPERATION
#include <stdint.h>  // for int32_t, int64_t
#include <QString>   // for QString
#include <map>       // for map
#include <vector>    // for vector

class QString;
namespace android {
namespace base {
class FunctorThread;
class System;
}  // namespace base
}  // namespace android
namespace emulator {
class VehiclePropValue;
}  // namespace emulator


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
