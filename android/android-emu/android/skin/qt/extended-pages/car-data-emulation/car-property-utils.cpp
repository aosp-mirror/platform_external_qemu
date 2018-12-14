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
#include <iomanip>
#include <sstream>

#include <QSettings>
#include <QDoubleValidator>
#define D(...) VERBOSE_PRINT(car, __VA_ARGS__)

using std::string;
using std::map;
using std::vector;
using emulator::EmulatorMessage;
using emulator::MsgType;
using emulator::Status;
using emulator::VehicleProperty;
using emulator::VehiclePropValue;
using emulator::VehicleGear;
using emulator::VehicleIgnitionState;
using emulator::VehiclePropertyType;
using emulator::VehicleArea;
using emulator::VehicleAreaSeat;
using emulator::VehicleAreaWindow;
using emulator::VehicleAreaDoor;
using emulator::VehicleAreaMirror;
using emulator::VehicleAreaWheel;
using emulator::VehicleLightState;
using emulator::VehicleLightSwitch;
using emulator::VehicleHvacFanDirection;
using emulator::VehicleUnit;
using emulator::VehicleOilLevel;
using emulator::FuelType;
using emulator::EvConnectorType;
using emulator::PortLocationType;
using emulator::VehicleApPowerStateReq;
using emulator::VehicleApPowerStateShutdownParam;
using emulator::VehicleApPowerStateReport;
using namespace CarPropertyUtils;

static constexpr int FLOAT_PRECISION = 3;

// All of these constants are derived from vehicle_constants_generated.h, so
// these 2 files have to stay in sync with each other.
map<int32_t, string> mirrorMap = {
    { (int32_t) VehicleAreaMirror::DRIVER_LEFT, "Driver left" },
    { (int32_t) VehicleAreaMirror::DRIVER_RIGHT, "Driver right" },
    { (int32_t) VehicleAreaMirror::DRIVER_CENTER, "Driver center" }
};

map<int32_t, string> seatMap = {
    { (int32_t) VehicleAreaSeat::ROW_1_LEFT, "Row 1 L" },
    { (int32_t) VehicleAreaSeat::ROW_1_CENTER, "Row 1 C" },
    { (int32_t) VehicleAreaSeat::ROW_1_RIGHT, "Row 1 R" },
    { (int32_t) VehicleAreaSeat::ROW_2_LEFT, "Row 2 L" },
    { (int32_t) VehicleAreaSeat::ROW_2_CENTER, "Row 2 C" },
    { (int32_t) VehicleAreaSeat::ROW_2_RIGHT, "Row 2 R" },
    { (int32_t) VehicleAreaSeat::ROW_3_LEFT, "Row 3 L" },
    { (int32_t) VehicleAreaSeat::ROW_3_CENTER, "Row 3 C" },
    { (int32_t) VehicleAreaSeat::ROW_3_RIGHT, "Row 3 R" }
};

map<int32_t, string> windowMap = {
    { (int32_t) VehicleAreaWindow::FRONT_WINDSHIELD, "Front windshield" },
    { (int32_t) VehicleAreaWindow::REAR_WINDSHIELD, "Rear windshield" },
    { (int32_t) VehicleAreaWindow::ROW_1_LEFT, "Row 1 L" },
    { (int32_t) VehicleAreaWindow::ROW_1_RIGHT, "Row 1 R" },
    { (int32_t) VehicleAreaWindow::ROW_2_LEFT, "Row 2 L" },
    { (int32_t) VehicleAreaWindow::ROW_2_RIGHT, "Row 2 R" },
    { (int32_t) VehicleAreaWindow::ROW_3_LEFT, "Row 3 L" },
    { (int32_t) VehicleAreaWindow::ROW_3_RIGHT, "Row 3 R" },
    { (int32_t) VehicleAreaWindow::ROOF_TOP_1, "Roof top 1" },
    { (int32_t) VehicleAreaWindow::ROOF_TOP_2, "Roof top 2" }
};

map<int32_t, string> doorMap = {
    { (int32_t) VehicleAreaDoor::ROW_1_LEFT, "Row 1 L" },
    { (int32_t) VehicleAreaDoor::ROW_1_RIGHT, "Row 1 R" },
    { (int32_t) VehicleAreaDoor::ROW_2_LEFT, "Row 2 L" },
    { (int32_t) VehicleAreaDoor::ROW_2_RIGHT, "Row 2 R" },
    { (int32_t) VehicleAreaDoor::ROW_3_LEFT, "Row 3 L" },
    { (int32_t) VehicleAreaDoor::ROW_3_RIGHT, "Row 3 R" },
    { (int32_t) VehicleAreaDoor::HOOD, "Hood" },
    { (int32_t) VehicleAreaDoor::REAR, "Rear" }
};

map<int32_t, string> wheelMap = {
    { (int32_t) VehicleAreaWheel::UNKNOWN, "Unknown" },
    { (int32_t) VehicleAreaWheel::LEFT_FRONT, "Front right" },
    { (int32_t) VehicleAreaWheel::RIGHT_FRONT, "Front right" },
    { (int32_t) VehicleAreaWheel::LEFT_REAR, "Rear left" },
    { (int32_t) VehicleAreaWheel::RIGHT_REAR, "Rear right" }
};

map<int32_t, string> ignitionStateMap = {
    { (int32_t) VehicleIgnitionState::UNDEFINED, "Undefined" },
    { (int32_t) VehicleIgnitionState::LOCK, "Locked" },
    { (int32_t) VehicleIgnitionState::OFF, "Off" },
    { (int32_t) VehicleIgnitionState::ACC, "Accessories available" },
    { (int32_t) VehicleIgnitionState::ON, "On" },
    { (int32_t) VehicleIgnitionState::START, "Engine starting" }
};

map<int32_t, string> gearMap = {
    { (int32_t) VehicleGear::GEAR_NEUTRAL, "N" },
    { (int32_t) VehicleGear::GEAR_REVERSE, "R" },
    { (int32_t) VehicleGear::GEAR_PARK, "P" },
    { (int32_t) VehicleGear::GEAR_DRIVE, "D" },
    { (int32_t) VehicleGear::GEAR_1, "D1" },
    { (int32_t) VehicleGear::GEAR_2, "D2" },
    { (int32_t) VehicleGear::GEAR_3, "D3" },
    { (int32_t) VehicleGear::GEAR_4, "D4" },
    { (int32_t) VehicleGear::GEAR_5, "D5" },
    { (int32_t) VehicleGear::GEAR_6, "D6" },
    { (int32_t) VehicleGear::GEAR_7, "D7" },
    { (int32_t) VehicleGear::GEAR_8, "D8" },
    { (int32_t) VehicleGear::GEAR_9, "D9" }
};

map<int32_t, string> lightStateMap = {
    { (int32_t) VehicleLightState::OFF, "Off" },
    { (int32_t) VehicleLightState::ON, "On" },
    { (int32_t) VehicleLightState::DAYTIME_RUNNING, "Daytime Running" }
};

map<int32_t, string> lightSwitchMap = {
    { (int32_t) VehicleLightSwitch::OFF, "Off" },
    { (int32_t) VehicleLightSwitch::ON, "On" },
    { (int32_t) VehicleLightSwitch::DAYTIME_RUNNING, "Daytime running lights" },
    { (int32_t) VehicleLightSwitch::AUTOMATIC, "Automatic" }
};

map<int32_t, string> tempUnitsMap = {
    { (int32_t) VehicleUnit::CELSIUS, "Celsius" },
    { (int32_t) VehicleUnit::FAHRENHEIT, "Fahrenheit" }
};

map<int32_t, string> fanDirectionMap = {
    { (int32_t) VehicleHvacFanDirection::FACE, "Face" },
    { (int32_t) VehicleHvacFanDirection::FLOOR, "Floor" },
    { (int32_t) VehicleHvacFanDirection::DEFROST, "Defrost" }
};

map<int32_t, string> oilLevelMap = {
    { (int32_t) VehicleOilLevel::CRITICALLY_LOW, "Critically Low" },
    { (int32_t) VehicleOilLevel::LOW, "Low" },
    { (int32_t) VehicleOilLevel::NORMAL, "Normal" },
    { (int32_t) VehicleOilLevel::HIGH, "High" },
    { (int32_t) VehicleOilLevel::ERROR, "Error" }
};

map<int32_t, string> fuelTypeMap = {
    { (int32_t) FuelType::FUEL_TYPE_UNKNOWN, "Unknown" },
    { (int32_t) FuelType::FUEL_TYPE_UNLEADED, "Unleaded" },
    { (int32_t) FuelType::FUEL_TYPE_DIESEL_1, "Diesel #1" },
    { (int32_t) FuelType::FUEL_TYPE_DIESEL_2, "Diesel #2" },
    { (int32_t) FuelType::FUEL_TYPE_BIODIESEL, "Biodiesel" },
    { (int32_t) FuelType::FUEL_TYPE_E85, "85\% Ethanol/Gasoline Blend" },
    { (int32_t) FuelType::FUEL_TYPE_LPG, "Liquified Petroleum Gas" },
    { (int32_t) FuelType::FUEL_TYPE_CNG, "Compressed Natural Gas" },
    { (int32_t) FuelType::FUEL_TYPE_LNG, "Liquified Natural Gas" },
    { (int32_t) FuelType::FUEL_TYPE_ELECTRIC, "Electric" },
    { (int32_t) FuelType::FUEL_TYPE_HYDROGEN, "Hydrogen" },
    { (int32_t) FuelType::FUEL_TYPE_OTHER, "Other" }
};

map<int32_t, string> evConnectorTypeMap = {
    { (int32_t) EvConnectorType::UNKNOWN, "Unknown" },
    { (int32_t) EvConnectorType::IEC_TYPE_1_AC, "IEC Type 1 AC" },
    { (int32_t) EvConnectorType::IEC_TYPE_2_AC, "IEC Type 2 AC" },
    { (int32_t) EvConnectorType::IEC_TYPE_3_AC, "IEC Type 3 AC" },
    { (int32_t) EvConnectorType::IEC_TYPE_4_DC, "IEC Type 4 DC" },
    { (int32_t) EvConnectorType::IEC_TYPE_1_CCS_DC, "IEC Type 1 CCS DC" },
    { (int32_t) EvConnectorType::IEC_TYPE_2_CCS_DC, "IEC Type 2 CCS DC" },
    { (int32_t) EvConnectorType::TESLA_ROADSTER, "Tesla Roadster" },
    { (int32_t) EvConnectorType::TESLA_HPWC, "Tesla HPWC" },
    { (int32_t) EvConnectorType::TESLA_SUPERCHARGER, "Tesla Supercharger" },
    { (int32_t) EvConnectorType::GBT_AC, "GBT AC" },
    { (int32_t) EvConnectorType::GBT_DC, "GBT DC" },
    { (int32_t) EvConnectorType::OTHER, "Other" }
};

map<int32_t, string> portLocationMap = {
    { (int32_t) PortLocationType::UNKNOWN, "Unknown" },
    { (int32_t) PortLocationType::FRONT_LEFT, "Front Left" },
    { (int32_t) PortLocationType::FRONT_RIGHT, "Front Right" },
    { (int32_t) PortLocationType::REAR_RIGHT, "Rear Right" },
    { (int32_t) PortLocationType::REAR_LEFT, "Rear Left" },
    { (int32_t) PortLocationType::FRONT, "Front" },
    { (int32_t) PortLocationType::REAR, "Rear" }
};

map<int32_t, string> apPowerStateReqMap = {
    { (int32_t) VehicleApPowerStateReq::OFF, "Off" },
    { (int32_t) VehicleApPowerStateReq::DEEP_SLEEP, "Deep Sleep" },
    { (int32_t) VehicleApPowerStateReq::ON_DISP_OFF, "AP On, Disp. Off" },
    { (int32_t) VehicleApPowerStateReq::ON_FULL, "Fully On" },
    { (int32_t) VehicleApPowerStateReq::SHUTDOWN_PREPARE, "Shutdown Requested" }
};

map<int32_t, string> apPowerStateShutdownMap = {
    { (int32_t) VehicleApPowerStateShutdownParam::SHUTDOWN_IMMEDIATELY, "Must shutdown immediately" },
    { (int32_t) VehicleApPowerStateShutdownParam::CAN_SLEEP, "Can enter deep sleep instead of shut down" },
    { (int32_t) VehicleApPowerStateShutdownParam::SHUTDOWN_ONLY, "Can only shutdown with postponing" }
};

map<int32_t, string> apPowerStateReportMap = {
    { (int32_t) VehicleApPowerStateReport::BOOT_COMPLETE, "Finished Booting" },
    { (int32_t) VehicleApPowerStateReport::DEEP_SLEEP_ENTRY, "Entering Deep Sleep" },
    { (int32_t) VehicleApPowerStateReport::DEEP_SLEEP_EXIT, "Exiting Deep Sleep" },
    { (int32_t) VehicleApPowerStateReport::SHUTDOWN_POSTPONE, "Shutdown Postponed" },
    { (int32_t) VehicleApPowerStateReport::SHUTDOWN_START, "Starting Shut Down" },
    { (int32_t) VehicleApPowerStateReport::DISPLAY_OFF, "Requested Disp. Off" },
    { (int32_t) VehicleApPowerStateReport::DISPLAY_ON, "Requested Disp. On" }
};

map<int32_t, PropWrapper> areaMap = {
    { (int32_t) VehicleArea::GLOBAL, { "Global" } },
    { (int32_t) VehicleArea::WINDOW, { "Window", &windowMap } },
    { (int32_t) VehicleArea::MIRROR, { "Mirror", &mirrorMap} },
    { (int32_t) VehicleArea::SEAT, { "Seat", &seatMap} },
    { (int32_t) VehicleArea::DOOR, { "Door", &doorMap} },
    { (int32_t) VehicleArea::WHEEL, { "Wheel", &wheelMap} }
};

map<int32_t, PropWrapper> CarPropertyUtils::propMap = {
    { 0, { "INVALID" } },
    { 286261504, { "VIN" } },
    { 286261505, { "Manufacturer" } },
    { 286261506, { "Model" } },
    { 289407235, { "Model Year" } },
    { 291504388, { "Fuel Capacity (mL)" } },
    { 289472773, { "Usable Fuels", &fuelTypeMap } },
    { 291504390, { "Battery Capacity (Wh)" } },
    { 289472775, { "Usable Connectors (EV)", &evConnectorTypeMap } },
    { 289407240, { "Fuel Door Location" } },
    { 289407241, { "EV Port Location", &portLocationMap } },
    { 356516106, { "Driver Seat Location", &seatMap } },
    { 291504644, { "Odometer Value (km)" } },
    { 291504647, { "Speed (m/s)" } },
    { 291504897, { "Engine Coolant Temperature (C)" } },
    { 289407747, { "Engine Oil Level", &oilLevelMap } },
    { 291504900, { "Engine Oil Temperature (C)" } },
    { 291504901, { "Engine RPM" } },
    { 290521862, { "Wheel Ticks" } },
    { 291504903, { "Fuel Level (mL)" } },
    { 287310600, { "Fuel Door Open" } },
    { 291504905, { "EV Battery Level (Wh)" } },
    { 287310602, { "EV Charge Port Open" } },
    { 287310603, { "EV Charge Port Connected" } },
    { 291504908, { "EV instantaneous charge rate (mW)" } },
    { 291504904, { "Range remaining (m)" } },
    { 392168201, { "Tire Pressure (kPa)" } },
    { 289408000, { "Selected Gear", &gearMap } },
    { 289408001, { "Current Gear", &gearMap } },
    { 287310850, { "Parking Brake On" } },
    { 287310851, { "Parking Brake Auto Apply On" } },
    { 287310853, { "Fuel Level Low" } },
    { 287310855, { "Night Mode On" } },
    { 289408008, { "Turn Signal State" } },
    { 289408009, { "Ignition State", &ignitionStateMap } },
    { 287310858, { "ABS Active" } },
    { 287310859, { "Traction Control Active" } },
    { 356517120, { "Fan Speed" } },
    { 356517121, { "Fan Direction", nullptr, fanDirectionToString } },
    { 358614274, { "Current Temperature (C)" } },
    { 358614275, { "Target Temperature Set (C)" } },
    { 320865540, { "Defroster On" } },
    { 354419973, { "AC On" } },
    { 354419974, { "Max AC On" } },
    { 354419975, { "Max Defrost On" } },
    { 354419976, { "Recirculation On" } },
    { 354419977, { "Temperature Coupling Enabled" } },
    { 354419978, { "Automatic Mode On" } },
    { 356517131, { "Seat Healing/Cooling", nullptr, heatingCoolingToString } },
    { 339739916, { "Side Mirror Heat" } },
    { 289408269, { "Steering Wheel Heating/Cooling", nullptr, heatingCoolingToString } },
    { 289408270, { "User Display Temperature Units", &tempUnitsMap } },
    { 356517135, { "Actual Fan Speed" } },
    { 354419984, { "Global Power State" } },
    { 356582673, { "Available Fan Positions", nullptr, fanDirectionToString } },
    { 354419986, { "Automatic Recirculation On" } },
    { 356517139, { "Seat Ventilation", nullptr, heatingCoolingToString } },
    { 291505923, { "Outside Temperature (C)" } },
    { 289475072, { "App Processor Power State Control", nullptr, nullptr, apPowerStateReqToString } },
    { 289475073, { "App Processor Power State Report", nullptr, nullptr, apPowerStateReportToString } },
    { 289409538, { "AP_POWER_BOOTUP_REASON" } },
    { 289409539, { "Display Brightness" } },
    { 289475088, { "H/W Input Key" } },
    { 373295872, { "Door Position" } },
    { 373295873, { "Door Move" } },
    { 371198722, { "Door Locked" } },
    { 339741504, { "Mirror Z Position" } },
    { 339741505, { "Mirror Z Move" } },
    { 339741506, { "Mirror Y Position" } },
    { 339741507, { "Mirror Y Move" } },
    { 287312708, { "Mirror Locked" } },
    { 287312709, { "Mirror Folded" } },
    { 356518784, { "Seat Selection Memory Preset" } },
    { 356518785, { "Seat Memory Set" } },
    { 354421634, { "Seatbelt Buckled" } },
    { 356518787, { "Seatbelt Height" } },
    { 356518788, { "Seatbelt Height Move" } },
    { 356518789, { "Seat Forward/Backward Position" } },
    { 356518790, { "Seat Forward/Backward Move" } },
    { 356518791, { "Seat Backrest Angle 1 Position" } },
    { 356518792, { "Seat Backrest Angle 1 Move" } },
    { 356518793, { "Seat Backrest Angle 2 Position" } },
    { 356518794, { "Seat Backrest Angle 2 Move" } },
    { 356518795, { "Seat Height Position" } },
    { 356518796, { "Seat Height Move" } },
    { 356518797, { "Seat Depth Position" } },
    { 356518798, { "Seat Depth Move" } },
    { 356518799, { "Seat Tilt Position" } },
    { 356518800, { "Seat Tilt Move" } },
    { 356518801, { "Lumbar Forward/Backward Position" } },
    { 356518802, { "Lumbar Forward/Backward Move" } },
    { 356518803, { "Lumbar Side Support Position" } },
    { 356518804, { "Lumbar Side Support Move" } },
    { 289409941, { "Headrest Height" } },
    { 356518806, { "Headrest Height Move" } },
    { 356518807, { "Headrest Angle" } },
    { 356518808, { "Headrest Angle Move" } },
    { 356518809, { "Headrest Forward/Backward Position" } },
    { 356518810, { "Headrest Forward/Backward Move" } },
    { 322964416, { "Window Position" } },
    { 322964417, { "Window Move" } },
    { 320867268, { "Window Locked" } },
    { 299895808, { "Vehicle Maps Service Msg" } },
    { 299896064, { "OBD2 Live Sensor Data" } },
    { 299896065, { "OBD2 Freeze Frame Sensor Data" } },
    { 299896066, { "OBD2 Freeze Frame Info" } },
    { 299896067, { "OBD2 Freeze Frame Clear" } },
    { 289410560, { "Headlights State", &lightStateMap } },
    { 289410561, { "High Beam Lights State", &lightStateMap } },
    { 289410562, { "Fog Lights State", &lightStateMap } },
    { 289410563, { "Hazard Lights State", &lightStateMap } },
    { 289410576, { "Headlights Switch ", &lightSwitchMap } },
    { 289410577, { "High Beam Lights Switch", &lightSwitchMap } },
    { 289410578, { "Fog Lights Switch", &lightSwitchMap } },
    { 289410579, { "Hazard Lights Switch", &lightSwitchMap } }
};

string CarPropertyUtils::getValueString(VehiclePropValue val) {
    string cellContents;
    vector<int32_t> int32Vals;
    vector<int64_t> int64Vals;
    vector<float> floatVals;
    PropWrapper prop = propMap[val.prop()];
    switch (val.value_type()) {
        case (int32_t) VehiclePropertyType::STRING :
            return val.string_value().c_str();

        case (int32_t) VehiclePropertyType::BOOLEAN :
            return booleanToString(prop, val.int32_values(0));

        case (int32_t) VehiclePropertyType::INT32 :
            return int32ToString(prop, val.int32_values(0));

        case (int32_t) VehiclePropertyType::INT32_VEC :
            for (int i = 0; i < val.int32_values_size(); i++) {
                int32Vals.push_back(val.int32_values(i));
            }
            return int32VecToString(prop, int32Vals);

        case (int32_t) VehiclePropertyType::INT64 :
            return std::to_string(val.int64_values(0));

        // TODO: Int64Vec/FloatVec - Currently only 1 property with either
        // (WHEEL_TICK is Int64Vec), and I think it is too complex to fit in a
        // small cell. Currently just printing the raw values.
        case (int32_t) VehiclePropertyType::INT64_VEC :
            for (int i = 0; i < val.int64_values_size(); i++) {
                int64Vals.push_back(val.int64_values(i));
            }
            return int64VecToString(prop, int64Vals);

        case (int32_t) VehiclePropertyType::FLOAT :
            return floatToString(prop, val.float_values(0));

        // TODO: See above
        case (int32_t) VehiclePropertyType::FLOAT_VEC :
            for (int i = 0; i < val.float_values_size(); i++) {
                floatVals.push_back(val.float_values(i));
            }
            return floatVecToString(prop, floatVals);

        // TODO: Bytes/Mixed - Figure out how to present relevant properties in
        // human readable form if possible - seem too complex to fit into table
        // cell. Bytes also currently has no relevant properties.
        case (int32_t) VehiclePropertyType::BYTES :
            return "Display N/A (Bytes)";

        case (int32_t) VehiclePropertyType::MIXED :
            return "Display N/A (Mixed)";

    }
    return "Unknown Value Type";
}

string CarPropertyUtils::getAreaString(VehiclePropValue val) {
    int maskedProp = val.prop() & (int) VehicleArea::MASK;
    PropWrapper prop = areaMap[maskedProp];
    string area = prop.label;
    if (prop.lookupTable != nullptr) {
        string details = CarPropertyUtils::multiDetailToString(*(prop.lookupTable), val.area_id());
        if (details != "") {
            area += ": " + CarPropertyUtils::multiDetailToString(*(prop.lookupTable), val.area_id());
        }
    }
    return area;
}

string CarPropertyUtils::booleanToString(PropWrapper prop, int32_t val) {
    return val ? "True" : "False";
}

string CarPropertyUtils::int32ToString(PropWrapper prop, int32_t val) {
    if (prop.lookupTable != nullptr) {
        return (*(prop.lookupTable))[val];
    } else if (prop.int32ToString != nullptr) {
        return prop.int32ToString(val);
    } else {
        return std::to_string(val);
    }
}

string CarPropertyUtils::int32VecToString(PropWrapper prop, vector<int32_t> vals) {
    // Vector is treated either treated specially as a group, or is just a list
    // of distinct values.
    if (prop.int32VecToString != nullptr) {
        return prop.int32VecToString(vals);
    } else  {
        string concat = string();
        for (int32_t val : vals) {
            concat += (int32ToString(prop, val) + "; ");
        }
        if (!concat.empty()) {
            concat.pop_back();
            concat.pop_back();
        }
        return concat;
    }
}

string CarPropertyUtils::int64ToString(PropWrapper prop, int64_t val) {
    // Currently no properties with this value type, so returning raw value.
    return std::to_string(val);
}

string CarPropertyUtils::int64VecToString(PropWrapper prop, vector<int64_t> vals) {
    // Currently the only property with this type is too complex to show in a
    // small cell, so just returning the raw values.
    string concat;
    for (int64_t val : vals) {
        concat += (int64ToString(prop, val) + "; ");
    }
    if (!concat.empty()) {
        concat.pop_back();
        concat.pop_back();
    }
    return concat;
}

string CarPropertyUtils::floatToString(PropWrapper prop, float val) {
    // Currently, the properties that use this value type only care about its
    // raw value, so that's what this returns.
    std::stringstream stream;
    stream << std::fixed << std::setprecision(FLOAT_PRECISION) << val;
    return stream.str();
}

string CarPropertyUtils::floatVecToString(PropWrapper prop, vector<float> vals) {
    // Currently no properties with this type, so just returning raw values.
    string concat;
    for (float val : vals) {
        concat += (floatToString(prop, val) + "; ");
    }
    if (!concat.empty()) {
        concat.pop_back();
        concat.pop_back();
    }
    return concat;
}

string CarPropertyUtils::multiDetailToString(map<int, string> lookupTable, int value) {
    string details = string();
    for (const auto &locDetail : lookupTable) {
        if ((value & locDetail.first) == locDetail.first) {
            details += (locDetail.second + ", ");
        }
    }
    if (!details.empty()) {
        details.pop_back();
        details.pop_back();
    }
    return details;
}

string CarPropertyUtils::fanDirectionToString(int val) {
    return multiDetailToString(fanDirectionMap, val);
}

string CarPropertyUtils::heatingCoolingToString(int val) {
    string behavior;
    if (val < 0) {
        behavior = "Cooling";
    } else if (val == 0) {
        behavior = "Off";
    } else {
        behavior = "On";
    }
    return behavior + " (" + std::to_string(val) + ")";
}

string CarPropertyUtils::apPowerStateReqToString(vector<int32_t> vals) {
    string output = apPowerStateReqMap[vals[0]];
    if (vals[0] == (int) VehicleApPowerStateReq::SHUTDOWN_PREPARE) {
        output += apPowerStateShutdownMap[vals[1]];
    }
    return output;
}

string CarPropertyUtils::apPowerStateReportToString(vector<int32_t> vals) {
    string output = apPowerStateReportMap[vals[0]];
    if (vals[0] == (int) VehicleApPowerStateReport::SHUTDOWN_POSTPONE) {
        output += "Time: " + std::to_string(vals[1]) + " ms";
    } else if (vals[0] == (int) VehicleApPowerStateReport::SHUTDOWN_START) {
        output += "Time to turn on AP: " + std::to_string(vals[1]) + " s";
    }
    return output;
}
