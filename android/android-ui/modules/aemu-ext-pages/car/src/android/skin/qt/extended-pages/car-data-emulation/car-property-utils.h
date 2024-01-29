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
        /**
         * Property name in String.
        */
        QString label;
        /**
         * How a property value will be translated into.
         * When lookupTableName is given, it is used as a key of lookupTablesMap
         * to get value->string mapping function. This string replaces
         * property value.
         *
         * e.g. if a property value is 48 ( = VehicleUnit::CELSIUS)
         * and "tempUnits" was set as lookupTableName, it will use tempUnitsMap
         * and value will be shown as "Celsius" instead of 48 in Vhal item
         *
         * When multiple lookupTableName exists, they represent non-overlapping
         * value->string mapping. For example, it might contain
         * [NormalState, ErrorState], where normal states are positive values,
         * and error states are negative.
        */
        std::vector<QString> lookupTableNames;
        /**
         * How a property value will be translated into.
         * Directly sets a value->string mapping function.
         * Resulted string will not replace the value, it will be printed
         * in the bracket next to the value.
         *
         * e.g. When a function heatingCoolingToString is given and property value is 0
         *      "0 (off)" is printed
        */
        QString (*int32ToString)(int32_t val);
        /**
         * How a property value will be translated into.
         * Directly sets a value->string mapping function, when value is a type of
         * a vector of intergers
        */
        QString (*int32VecToString)(std::vector<int32_t> vals);

        /**
         * The detailed description for the property.
         */
        QString detailedDescription;
    };

    // Map from property ids to description for conversion to human readable form.
    extern std::map<int32_t, PropertyDescription> propMap;
    extern std::map<QString, const std::map<int32_t, QString>*> lookupTablesMap;

    QString booleanToString(PropertyDescription prop, int32_t val);
    QString int32ToString(PropertyDescription prop, int32_t val);
    QString int32VecToString(PropertyDescription prop, std::vector<int32_t> vals);
    QString int64ToString(PropertyDescription prop, int64_t val);
    QString int64VecToString(PropertyDescription prop, std::vector<int64_t> vals);
    QString floatToString(PropertyDescription prop, float val);
    QString floatVecToString(PropertyDescription prop, std::vector<float> vals);
    QString int32ToHexString(int32_t val);

    QString getValueString(emulator::VehiclePropValue val);
    QString getDetailedDescription(int32_t propId);
    QString getAreaString(emulator::VehiclePropValue val);

    QString multiDetailToString(const std::map<int, QString>& lookupTable, int value);
    QString fanDirectionToString(int32_t val);
    QString heatingCoolingToString(int32_t val);
    QString apPowerStateReqToString(std::vector<int32_t> vals);
    QString apPowerStateReportToString(std::vector<int32_t> vals);
    QString vendorIdToString(int32_t val);
    QString changeModeToString(int32_t val);
    QString translate(const QString& src);
    bool isVendor(int32_t val);
    void loadDescriptionsFromJson(const char *filename);

}  // namespace carpropertyutils
