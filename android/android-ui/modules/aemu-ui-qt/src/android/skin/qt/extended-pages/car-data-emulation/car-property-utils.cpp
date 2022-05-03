// Copyright (°C) 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, disQObject::tributed, and modified under those terms.
//
// This program is disQObject::tributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR, A PARTICULAR, PURPOSE.  See the
// GNU General Public License for more details.
#include "android/skin/qt/extended-pages/car-data-emulation/car-property-utils.h"

#include <qstring.h>
#include <QByteArray>
#include <QObject>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <cstdint>
#include <sstream>
#include <string>
#include <utility>

#include "VehicleHalProto.pb.h"
#include "android/skin/qt/extended-pages/car-data-emulation/vehicle_constants_generated.h"
#include "android/utils/debug.h"
#include "aemu/base/Log.h"                               // for LogStream...

using std::map;
using std::vector;
using std::stringstream;
using std::hex;
using emulator::EmulatorMessage;
using emulator::MsgType;
using emulator::Status;
using emulator::VehicleProperty;
using emulator::VehiclePropertyGroup;
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
using emulator::GsrComplianceRequirementType;
using emulator::PortLocationType;
using emulator::VehicleApPowerStateReq;
using emulator::VehicleApPowerStateShutdownParam;
using emulator::VehicleApPowerStateReport;
using namespace carpropertyutils;

// All of these constants are derived from vehicle_constants_generated.h, so
// these 2 files have to stay in sync with each other.
map<int32_t, QString> mirrorMap = {
    { (int32_t) VehicleAreaMirror::DRIVER_LEFT, QObject::tr("Driver left") },
    { (int32_t) VehicleAreaMirror::DRIVER_RIGHT, QObject::tr("Driver right") },
    { (int32_t) VehicleAreaMirror::DRIVER_CENTER, QObject::tr("Driver center") }
};

map<int32_t, QString> seatMap = {
    { (int32_t) VehicleAreaSeat::ROW_1_LEFT, QObject::tr("Row 1 L") },
    { (int32_t) VehicleAreaSeat::ROW_1_CENTER, QObject::tr("Row 1 C") },
    { (int32_t) VehicleAreaSeat::ROW_1_RIGHT, QObject::tr("Row 1 R") },
    { (int32_t) VehicleAreaSeat::ROW_2_LEFT, QObject::tr("Row 2 L") },
    { (int32_t) VehicleAreaSeat::ROW_2_CENTER, QObject::tr("Row 2 C") },
    { (int32_t) VehicleAreaSeat::ROW_2_RIGHT, QObject::tr("Row 2 R") },
    { (int32_t) VehicleAreaSeat::ROW_3_LEFT, QObject::tr("Row 3 L") },
    { (int32_t) VehicleAreaSeat::ROW_3_CENTER, QObject::tr("Row 3 C") },
    { (int32_t) VehicleAreaSeat::ROW_3_RIGHT, QObject::tr("Row 3 R") }
};

map<int32_t, QString> windowMap = {
    { (int32_t) VehicleAreaWindow::FRONT_WINDSHIELD, QObject::tr("Front windshield") },
    { (int32_t) VehicleAreaWindow::REAR_WINDSHIELD, QObject::tr("Rear windshield") },
    { (int32_t) VehicleAreaWindow::ROW_1_LEFT, QObject::tr("Row 1 L") },
    { (int32_t) VehicleAreaWindow::ROW_1_RIGHT, QObject::tr("Row 1 R") },
    { (int32_t) VehicleAreaWindow::ROW_2_LEFT, QObject::tr("Row 2 L") },
    { (int32_t) VehicleAreaWindow::ROW_2_RIGHT, QObject::tr("Row 2 R") },
    { (int32_t) VehicleAreaWindow::ROW_3_LEFT, QObject::tr("Row 3 L") },
    { (int32_t) VehicleAreaWindow::ROW_3_RIGHT, QObject::tr("Row 3 R") },
    { (int32_t) VehicleAreaWindow::ROOF_TOP_1, QObject::tr("Roof top 1") },
    { (int32_t) VehicleAreaWindow::ROOF_TOP_2, QObject::tr("Roof top 2") }
};

map<int32_t, QString> doorMap = {
    { (int32_t) VehicleAreaDoor::ROW_1_LEFT, QObject::tr("Row 1 L") },
    { (int32_t) VehicleAreaDoor::ROW_1_RIGHT, QObject::tr("Row 1 R") },
    { (int32_t) VehicleAreaDoor::ROW_2_LEFT, QObject::tr("Row 2 L") },
    { (int32_t) VehicleAreaDoor::ROW_2_RIGHT, QObject::tr("Row 2 R") },
    { (int32_t) VehicleAreaDoor::ROW_3_LEFT, QObject::tr("Row 3 L") },
    { (int32_t) VehicleAreaDoor::ROW_3_RIGHT, QObject::tr("Row 3 R") },
    { (int32_t) VehicleAreaDoor::HOOD, QObject::tr("Hood") },
    { (int32_t) VehicleAreaDoor::REAR, QObject::tr("Rear") }
};

map<int32_t, QString> wheelMap = {
    { (int32_t) VehicleAreaWheel::UNKNOWN, QObject::tr("Unknown") },
    { (int32_t) VehicleAreaWheel::LEFT_FRONT, QObject::tr("Front left") },
    { (int32_t) VehicleAreaWheel::RIGHT_FRONT, QObject::tr("Front right") },
    { (int32_t) VehicleAreaWheel::LEFT_REAR, QObject::tr("Rear left") },
    { (int32_t) VehicleAreaWheel::RIGHT_REAR, QObject::tr("Rear right") }
};

map<int32_t, QString> ignitionStateMap = {
    { (int32_t) VehicleIgnitionState::UNDEFINED, QObject::tr("Undefined") },
    { (int32_t) VehicleIgnitionState::LOCK, QObject::tr("Locked") },
    { (int32_t) VehicleIgnitionState::OFF, QObject::tr("Off") },
    { (int32_t) VehicleIgnitionState::ACC, QObject::tr("Accessories available") },
    { (int32_t) VehicleIgnitionState::ON, QObject::tr("On") },
    { (int32_t) VehicleIgnitionState::START, QObject::tr("Engine starting") }
};

map<int32_t, QString> gearMap = {
    { (int32_t) VehicleGear::GEAR_NEUTRAL, QObject::tr("N") },
    { (int32_t) VehicleGear::GEAR_REVERSE, QObject::tr("R") },
    { (int32_t) VehicleGear::GEAR_PARK, QObject::tr("P") },
    { (int32_t) VehicleGear::GEAR_DRIVE, QObject::tr("D") },
    { (int32_t) VehicleGear::GEAR_1, QObject::tr("D1") },
    { (int32_t) VehicleGear::GEAR_2, QObject::tr("D2") },
    { (int32_t) VehicleGear::GEAR_3, QObject::tr("D3") },
    { (int32_t) VehicleGear::GEAR_4, QObject::tr("D4") },
    { (int32_t) VehicleGear::GEAR_5, QObject::tr("D5") },
    { (int32_t) VehicleGear::GEAR_6, QObject::tr("D6") },
    { (int32_t) VehicleGear::GEAR_7, QObject::tr("D7") },
    { (int32_t) VehicleGear::GEAR_8, QObject::tr("D8") },
    { (int32_t) VehicleGear::GEAR_9, QObject::tr("D9") }
};

map<int32_t, QString> lightStateMap = {
    { (int32_t) VehicleLightState::OFF, QObject::tr("Off") },
    { (int32_t) VehicleLightState::ON, QObject::tr("On") },
    { (int32_t) VehicleLightState::DAYTIME_RUNNING, QObject::tr("Daytime running") }
};

map<int32_t, QString> lightSwitchMap = {
    { (int32_t) VehicleLightSwitch::OFF, QObject::tr("Off") },
    { (int32_t) VehicleLightSwitch::ON, QObject::tr("On") },
    { (int32_t) VehicleLightSwitch::DAYTIME_RUNNING, QObject::tr("Daytime running lights") },
    { (int32_t) VehicleLightSwitch::AUTOMATIC, QObject::tr("Automatic") }
};

map<int32_t, QString> tempUnitsMap = {
    { (int32_t) VehicleUnit::CELSIUS, QObject::tr("Celsius") },
    { (int32_t) VehicleUnit::FAHRENHEIT, QObject::tr("Fahrenheit") }
};

map<int32_t, QString> fanDirectionMap = {
    { (int32_t) VehicleHvacFanDirection::FACE, QObject::tr("Face") },
    { (int32_t) VehicleHvacFanDirection::FLOOR, QObject::tr("Floor") },
    { (int32_t) VehicleHvacFanDirection::DEFROST, QObject::tr("Defrost") }
};

map<int32_t, QString> oilLevelMap = {
    { (int32_t) VehicleOilLevel::CRITICALLY_LOW, QObject::tr("Critically low") },
    { (int32_t) VehicleOilLevel::LOW, QObject::tr("Low") },
    { (int32_t) VehicleOilLevel::NORMAL, QObject::tr("Normal") },
    { (int32_t) VehicleOilLevel::HIGH, QObject::tr("High") },
    { (int32_t) VehicleOilLevel::ERROR, QObject::tr("Error") }
};

map<int32_t, QString> fuelTypeMap = {
    { (int32_t) FuelType::FUEL_TYPE_UNKNOWN, QObject::tr("Unknown") },
    { (int32_t) FuelType::FUEL_TYPE_UNLEADED, QObject::tr("Unleaded") },
    { (int32_t) FuelType::FUEL_TYPE_DIESEL_1, QObject::tr("Diesel #1") },
    { (int32_t) FuelType::FUEL_TYPE_DIESEL_2, QObject::tr("Diesel #2") },
    { (int32_t) FuelType::FUEL_TYPE_BIODIESEL, QObject::tr("Biodiesel") },
    { (int32_t) FuelType::FUEL_TYPE_E85, QObject::tr("85\% ethanol/gasoline blend") },
    { (int32_t) FuelType::FUEL_TYPE_LPG, QObject::tr("Liquified petroleum gas") },
    { (int32_t) FuelType::FUEL_TYPE_CNG, QObject::tr("Compressed natural gas") },
    { (int32_t) FuelType::FUEL_TYPE_LNG, QObject::tr("Liquified natural gas") },
    { (int32_t) FuelType::FUEL_TYPE_ELECTRIC, QObject::tr("Electric") },
    { (int32_t) FuelType::FUEL_TYPE_HYDROGEN, QObject::tr("Hydrogen") },
    { (int32_t) FuelType::FUEL_TYPE_OTHER, QObject::tr("Other") }
};

map<int32_t, QString> evConnectorTypeMap = {
    { (int32_t) EvConnectorType::UNKNOWN, QObject::tr("Unknown") },
    { (int32_t) EvConnectorType::IEC_TYPE_1_AC, QObject::tr("IEC type 1 AC") },
    { (int32_t) EvConnectorType::IEC_TYPE_2_AC, QObject::tr("IEC type 2 AC") },
    { (int32_t) EvConnectorType::IEC_TYPE_3_AC, QObject::tr("IEC type 3 AC") },
    { (int32_t) EvConnectorType::IEC_TYPE_4_DC, QObject::tr("IEC type 4 DC") },
    { (int32_t) EvConnectorType::IEC_TYPE_1_CCS_DC, QObject::tr("IEC type 1 CCS DC") },
    { (int32_t) EvConnectorType::IEC_TYPE_2_CCS_DC, QObject::tr("IEC type 2 CCS DC") },
    { (int32_t) EvConnectorType::TESLA_ROADSTER, QObject::tr("Tesla Roadster") },
    { (int32_t) EvConnectorType::TESLA_HPWC, QObject::tr("Tesla HPWC") },
    { (int32_t) EvConnectorType::TESLA_SUPERCHARGER, QObject::tr("Tesla Supercharger") },
    { (int32_t) EvConnectorType::GBT_AC, QObject::tr("GBT AC") },
    { (int32_t) EvConnectorType::GBT_DC, QObject::tr("GBT DC") },
    { (int32_t) EvConnectorType::OTHER, QObject::tr("Other") }
};

map<int32_t, QString> portLocationMap = {
    { (int32_t) PortLocationType::UNKNOWN, QObject::tr("Unknown") },
    { (int32_t) PortLocationType::FRONT_LEFT, QObject::tr("Front left") },
    { (int32_t) PortLocationType::FRONT_RIGHT, QObject::tr("Front right") },
    { (int32_t) PortLocationType::REAR_RIGHT, QObject::tr("Rear right") },
    { (int32_t) PortLocationType::REAR_LEFT, QObject::tr("Rear left") },
    { (int32_t) PortLocationType::FRONT, QObject::tr("Front") },
    { (int32_t) PortLocationType::REAR, QObject::tr("Rear") }
};

map<int32_t, QString> apPowerStateReqMap = {
    { (int32_t) VehicleApPowerStateReq::ON, QObject::tr("Fully on") },
    { (int32_t) VehicleApPowerStateReq::SHUTDOWN_PREPARE, QObject::tr("Shutdown requested") },
    { (int32_t) VehicleApPowerStateReq::CANCEL_SHUTDOWN, QObject::tr("Shutdown canceled") },
    { (int32_t) VehicleApPowerStateReq::FINISHED, QObject::tr("VHAL is ready for shutdown") },
};

map<int32_t, QString> apPowerStateShutdownMap = {
    { (int32_t) VehicleApPowerStateShutdownParam::SHUTDOWN_IMMEDIATELY,
            QObject::tr("Must shutdown immediately") },
    { (int32_t) VehicleApPowerStateShutdownParam::CAN_SLEEP,
            QObject::tr("Enter deep sleep after Garage Mode") },
    { (int32_t) VehicleApPowerStateShutdownParam::SHUTDOWN_ONLY,
            QObject::tr("Shutdown after Garage Mode") },
    { (int32_t) VehicleApPowerStateShutdownParam::SLEEP_IMMEDIATELY,
            QObject::tr("Enter deep sleep immediately") },
    { (int32_t) VehicleApPowerStateShutdownParam::HIBERNATE_IMMEDIATELY,
            QObject::tr("Enter hibernation immediately") },
    { (int32_t) VehicleApPowerStateShutdownParam::CAN_HIBERNATE,
            QObject::tr("Enter hibernation after Garage Mode") }
};

map<int32_t, QString> apPowerStateReportMap = {
    { (int32_t) VehicleApPowerStateReport::WAIT_FOR_VHAL, QObject::tr("Waiting for VHAL") },
    { (int32_t) VehicleApPowerStateReport::DEEP_SLEEP_ENTRY, QObject::tr("Entering deep sleep") },
    { (int32_t) VehicleApPowerStateReport::DEEP_SLEEP_EXIT, QObject::tr("Exiting deep sleep") },
    { (int32_t) VehicleApPowerStateReport::SHUTDOWN_POSTPONE, QObject::tr("Shutdown postponed") },
    { (int32_t) VehicleApPowerStateReport::SHUTDOWN_START, QObject::tr("Starting shut down") },
    { (int32_t) VehicleApPowerStateReport::ON, QObject::tr("Device on") },
    { (int32_t) VehicleApPowerStateReport::SHUTDOWN_PREPARE, QObject::tr("Shutdown request received") },
    { (int32_t) VehicleApPowerStateReport::SHUTDOWN_CANCELLED, QObject::tr("Shutdown cancel received") },
    { (int32_t) VehicleApPowerStateReport::HIBERNATION_ENTRY, QObject::tr("Entering hibernation") },
    { (int32_t) VehicleApPowerStateReport::HIBERNATION_EXIT, QObject::tr("Exiting hibernation") }
};

map<int32_t, PropertyDescription> areaMap = {
    { (int32_t) VehicleArea::GLOBAL, { QObject::tr("Global") } },
    { (int32_t) VehicleArea::WINDOW, { QObject::tr("Window"), "window" } },
    { (int32_t) VehicleArea::MIRROR, { QObject::tr("Mirror"), "mirror" } },
    { (int32_t) VehicleArea::SEAT, { QObject::tr("Seat"), "seat" } },
    { (int32_t) VehicleArea::DOOR, { QObject::tr("Door"), "door" } },
    { (int32_t) VehicleArea::WHEEL, { QObject::tr("Wheel"), "wheel" } }
};

map<int32_t, QString> gsrComplianceReqMap = {
    { (int32_t) GsrComplianceRequirementType::GSR_COMPLIANCE_NOT_REQUIRED,
      QObject::tr("GSR compliance not required") },
    { (int32_t) GsrComplianceRequirementType::GSR_COMPLIANCE_REQUIRED_THROUGH_SYSTEM_IMAGE,
      QObject::tr("GSR compliance required through system image") }
};

map<QString, std::map<int32_t, QString>*> carpropertyutils::lookupTablesMap = {
    { "fuelType", &fuelTypeMap },
    { "evConnector", &evConnectorTypeMap },
    { "portLocation", &portLocationMap },
    { "seat", &seatMap },
    { "mirror", &mirrorMap },
    { "door", &doorMap },
    { "window", &windowMap },
    { "oilLevel", &oilLevelMap },
    { "gear", &gearMap },
    { "wheel", &wheelMap },
    { "ignitionState", &ignitionStateMap },
    { "fanDirection", &fanDirectionMap },
    { "tempUnits", &tempUnitsMap },
    { "lightState", &lightStateMap },
    { "lightSwitch", &lightSwitchMap },
    { "gsrComplianceReq",  &gsrComplianceReqMap },
};

map<int32_t, PropertyDescription> carpropertyutils::propMap = {
    { 0,         { QObject::tr("INVALID") } },
    { 286261504, { QObject::tr("VIN") } },
    { 286261505, { QObject::tr("Manufacturer") } },
    { 286261506, { QObject::tr("Model") } },
    { 289407235, { QObject::tr("Model year") } },
    { 291504388, { QObject::tr("Fuel capacity (mL)") } },
    { 289472773, { QObject::tr("Usable fuels"), "fuelType" } },
    { 291504390, { QObject::tr("Battery capacity (Wh)") } },
    { 289472775, { QObject::tr("Usable connectors (EV)"), "evConnector" } },
    { 289407240, { QObject::tr("Fuel door location") } },
    { 289407241, { QObject::tr("EV port location"), "portLocation" } },
    { 356516106, { QObject::tr("Driver seat location"), "seat" } },
    { 291504644, { QObject::tr("Odometer value (km)") } },
    { 291504647, { QObject::tr("Speed (m/s)") } },
    { 291504897, { QObject::tr("Engine coolant temperature (°C)") } },
    { 289407747, { QObject::tr("Engine oil level"), "oilLevel" } },
    { 291504900, { QObject::tr("Engine oil temperature (°C)") } },
    { 291504901, { QObject::tr("Engine RPM") } },
    { 290521862, { QObject::tr("Wheel ticks") } },
    { 291504903, { QObject::tr("Fuel level (mL)") } },
    { 287310600, { QObject::tr("Fuel door open") } },
    { 291504905, { QObject::tr("EV battery level (Wh)") } },
    { 287310602, { QObject::tr("EV charge port open") } },
    { 287310603, { QObject::tr("EV charge port connected") } },
    { 291504908, { QObject::tr("EV instantaneous charge rate (mW)") } },
    { 291504904, { QObject::tr("Range remaining (m)") } },
    { 392168201, { QObject::tr("Tire pressure (kPa)") } },
    { 289408000, { QObject::tr("Selected gear"), "gear" } },
    { 289408001, { QObject::tr("Current gear"), "gear" } },
    { 287310850, { QObject::tr("Parking brake on") } },
    { 287310851, { QObject::tr("Parking brake auto apply on") } },
    { 287310853, { QObject::tr("Fuel level low") } },
    { 287310855, { QObject::tr("Night mode on") } },
    { 289408008, { QObject::tr("Turn signal state") } },
    { 289408009, { QObject::tr("Ignition state"), "ignitionState" } },
    { 287310858, { QObject::tr("ABS active") } },
    { 287310859, { QObject::tr("Traction control active") } },
    { 356517120, { QObject::tr("Fan speed") } },
    { 356517121, { QObject::tr("Fan direction"), "fanDirection", fanDirectionToString } },
    { 358614274, { QObject::tr("Current temperature (°C)") } },
    { 358614275, { QObject::tr("Target temperature set (°C)") } },
    { 320865540, { QObject::tr("Defroster on") } },
    { 354419973, { QObject::tr("AC on") } },
    { 354419974, { QObject::tr("Max AC on") } },
    { 354419975, { QObject::tr("Max defrost on") } },
    { 354419976, { QObject::tr("Recirculation on") } },
    { 354419977, { QObject::tr("Temperature coupling enabled") } },
    { 354419978, { QObject::tr("Automatic mode on") } },
    { 356517131, { QObject::tr("Seat healing/cooling") , nullptr, heatingCoolingToString } },
    { 339739916, { QObject::tr("Side mirror heat") } },
    { 289408269, { QObject::tr("Steering wheel heating/cooling"),
            nullptr, heatingCoolingToString } },
    { 289408270, { QObject::tr("User display temperature units"), "tempUnits" } },
    { 356517135, { QObject::tr("Actual fan speed") } },
    { 354419984, { QObject::tr("Global power state") } },
    { 356582673, { QObject::tr("Available fan positions"), "fanDirection", fanDirectionToString } },
    { 354419986, { QObject::tr("Automatic recirculation on") } },
    { 356517139, { QObject::tr("Seat ventilation"), nullptr, heatingCoolingToString } },
    { 291505923, { QObject::tr("Outside temperature (°C)") } },
    { 289475072, { QObject::tr("App processor power state control"),
            nullptr, nullptr, apPowerStateReqToString } },
    { 289475073, { QObject::tr("App processor power state report"),
            nullptr, nullptr, apPowerStateReportToString } },
    { 289409538, { QObject::tr("Bootup reason for power on") } },
    { 289409539, { QObject::tr("Display brightness") } },
    { 289475088, { QObject::tr("H/W input key") } },
    { 373295872, { QObject::tr("Door position") } },
    { 373295873, { QObject::tr("Door move") } },
    { 371198722, { QObject::tr("Door locked") } },
    { 339741504, { QObject::tr("Mirror Z position") } },
    { 339741505, { QObject::tr("Mirror Z move") } },
    { 339741506, { QObject::tr("Mirror Y position") } },
    { 339741507, { QObject::tr("Mirror Y move") } },
    { 287312708, { QObject::tr("Mirror locked") } },
    { 287312709, { QObject::tr("Mirror folded") } },
    { 356518784, { QObject::tr("Seat selection memory preset") } },
    { 356518785, { QObject::tr("Seat memory set") } },
    { 354421634, { QObject::tr("Seatbelt buckled") } },
    { 356518787, { QObject::tr("Seatbelt height") } },
    { 356518788, { QObject::tr("Seatbelt height move") } },
    { 356518789, { QObject::tr("Seat forward/backward position") } },
    { 356518790, { QObject::tr("Seat forward/backward move") } },
    { 356518791, { QObject::tr("Seat backrest angle 1 position") } },
    { 356518792, { QObject::tr("Seat backrest angle 1 move") } },
    { 356518793, { QObject::tr("Seat backrest angle 2 position") } },
    { 356518794, { QObject::tr("Seat backrest angle 2 move") } },
    { 356518795, { QObject::tr("Seat height position") } },
    { 356518796, { QObject::tr("Seat height move") } },
    { 356518797, { QObject::tr("Seat depth position") } },
    { 356518798, { QObject::tr("Seat depth move") } },
    { 356518799, { QObject::tr("Seat tilt position") } },
    { 356518800, { QObject::tr("Seat tilt move") } },
    { 356518801, { QObject::tr("Lumbar forward/backward position") } },
    { 356518802, { QObject::tr("Lumbar forward/backward move") } },
    { 356518803, { QObject::tr("Lumbar side support position") } },
    { 356518804, { QObject::tr("Lumbar side support move") } },
    { 289409941, { QObject::tr("Headrest height") } },
    { 356518806, { QObject::tr("Headrest height move") } },
    { 356518807, { QObject::tr("Headrest angle") } },
    { 356518808, { QObject::tr("Headrest angle move") } },
    { 356518809, { QObject::tr("Headrest forward/backward position") } },
    { 356518810, { QObject::tr("Headrest forward/backward move") } },
    { 322964416, { QObject::tr("Window position") } },
    { 322964417, { QObject::tr("Window move") } },
    { 320867268, { QObject::tr("Window locked") } },
    { 299895808, { QObject::tr("Vehicle maps service msg") } },
    { 299896064, { QObject::tr("OBD2 live sensor data") } },
    { 299896065, { QObject::tr("OBD2 freeze frame sensor data") } },
    { 299896066, { QObject::tr("OBD2 freeze frame info") } },
    { 299896067, { QObject::tr("OBD2 freeze frame clear") } },
    { 289410560, { QObject::tr("Headlights state"), "lightState" } },
    { 289410561, { QObject::tr("High beam lights state"), "lightState" } },
    { 289410562, { QObject::tr("Fog lights state"), "lightState" } },
    { 289410563, { QObject::tr("Hazard lights state"), "lightState" } },
    { 289410576, { QObject::tr("Headlights switch"), "lightSwitch" } },
    { 289410577, { QObject::tr("High beam lights switch"), "lightSwitch" } },
    { 289410578, { QObject::tr("Fog lights switch"), "lightSwitch" } },
    { 289410579, { QObject::tr("Hazard lights switch"), "lightSwitch" } },
    { 289410887, { QObject::tr("EU General Safety Regulation (GSR) Compliance Requirement"), "gsrComplianceReq" } },
    { 289472779, { QObject::tr("Exterior dimensions (mm)") } },
    { 289472780, { QObject::tr("List of fuel door locations") } },
    { 291504648, { QObject::tr("Speed for displays (m/s)") } },
    { 291504649, { QObject::tr("Front bicycle model steering angle (°)") } },
    { 291504656, { QObject::tr("Rear bicycle model steering angle (°)") } },
    { 392168202, { QObject::tr("Critically low tire pressure") } },
    { 320865556, { QObject::tr("Electric defroster") } },
    { 291570965, { QObject::tr("Suggested temperature set (°C)") } },
    { 289408512, { QObject::tr("Distance units for display") } },
    { 289408513, { QObject::tr("Fuel volume units for display") } },
    { 289408514, { QObject::tr("Tire pressure units for display") } },
    { 289408515, { QObject::tr("EV battery units for display") } },
    { 287311364, { QObject::tr("Fuel consumption units for display") } },
    { 289408517, { QObject::tr("Speed units for display") } },
    { 290457096, { QObject::tr("Current time in car (Epoch)") } },
    { 290457094, { QObject::tr("Current time in android (Epoch)") } },
    { 292554247, { QObject::tr("External encryption binding seed") } },
    { 289475104, { QObject::tr("H/W rotary events") } },
    { 289475120, { QObject::tr("Custom OEM partner input") } },
    { 356518832, { QObject::tr("Seat Occupancy") } },
    { 289410817, { QObject::tr("Cabin lights") } },
    { 289410818, { QObject::tr("Cabin lights switch") } },
    { 356519683, { QObject::tr("Reading lights") } },
    { 356519684, { QObject::tr("Reading lights switch") } },
    { 287313669, { QObject::tr("Support customize permissions for vendor properties") } },
    { 286265094, { QObject::tr("Allow disabling optional featurs from vhal") } },
    { 299896583, { QObject::tr("Android user during initialization") } },
    { 299896584, { QObject::tr("Request to switch the foreground Android user") } },
    { 299896585, { QObject::tr("Android user was created") } },
    { 299896586, { QObject::tr("Android user was removed") } },
    { 299896587, { QObject::tr("Associate the current user with vehicle-specific identification") } },
    { 289476368, { QObject::tr("Enable/request an EVS service") } },
    { 286265121, { QObject::tr("Request to apply power policy") } },
    { 286265122, { QObject::tr("Request to set the power polic group") } },
    { 286265123, { QObject::tr("Current power policy") } },
    { 290459441, { QObject::tr("Event telling the car watchdog alive") } },
    { 299896626, { QObject::tr("Process terminated by car watchdog and the reason") } },
    { 290459443, { QObject::tr("Event VHAL signals to car watchdog as a heartbeat") } },
    { 289410868, { QObject::tr("Starts the ClusterUI") } },
    { 289476405, { QObject::tr("Changes the state of the cluster display") } },
    { 299896630, { QObject::tr("Reports the current display state and ClusterUI state") } },
    { 289410871, { QObject::tr("Requests to change the cluster display state to show some ClusterUI") } },
    { 292556600, { QObject::tr("Current cluster navigation state") } },
    { 289410873, { QObject::tr("Electronic Toll Collection card type") } },
    { 289410874, { QObject::tr("Electronic Toll Collection card status") } },
    { 289410875, { QObject::tr("Front fog lights state") } },
    { 289410876, { QObject::tr("Front fog lights switch") } },
    { 289410877, { QObject::tr("Rear fog lights state") } },
    { 289410878, { QObject::tr("Rear fog lights switch") } },
    { 291508031, { QObject::tr("EV maximum current draw threshold for charging set by the user") } },
    { 291508032, { QObject::tr("EV maximum charge percent threshold set by the user") } },
    { 289410881, { QObject::tr("EV Charging state") } },
    { 287313730, { QObject::tr("EV Start or stop charging the battery") } },
    { 289410883, { QObject::tr("EV Estimated charge time remaining (seconds)") } },
    { 289410884, { QObject::tr("EV Regenerative braking or one-pedal drive state") } },
    { 289410885, { QObject::tr("Trailer present or not") } },
    { 289410886, { QObject::tr("Vehicle curb weight") } },
};

QString carpropertyutils::getValueString(VehiclePropValue val) {
    QString cellContents;
    vector<int32_t> int32Vals;
    vector<int64_t> int64Vals;
    vector<float> floatVals;
    PropertyDescription prop = propMap[val.prop()];
    switch (val.value_type()) {
        case (int32_t) VehiclePropertyType::STRING :
            return QObject::tr(val.string_value().data());

        case (int32_t) VehiclePropertyType::BOOLEAN :
            if (val.int32_values_size() > 0) {
                return booleanToString(prop, val.int32_values(0));
            } else {
                return "N/A";
            }

        case (int32_t) VehiclePropertyType::INT32 :
            if (val.int32_values_size() > 0) {
                return int32ToString(prop, val.int32_values(0));
            } else {
                return "N/A";
            }

        case (int32_t) VehiclePropertyType::INT32_VEC :
            for (int i = 0; i < val.int32_values_size(); i++) {
                int32Vals.push_back(val.int32_values(i));
            }
            return int32VecToString(prop, int32Vals);

        case (int32_t) VehiclePropertyType::INT64 :
            if (val.int64_values_size() > 0) {
                return int64ToString(prop, val.int64_values(0));
            } else {
                return "N/A";
            }

        // TODO: Int64Vec/FloatVec - Currently only 1 property with either
        // (WHEEL_TICK is Int64Vec), and I think it is too complex to fit in a
        // small cell. Currently just printing the raw values.
        case (int32_t) VehiclePropertyType::INT64_VEC :
            for (int i = 0; i < val.int64_values_size(); i++) {
                int64Vals.push_back(val.int64_values(i));
            }
            return int64VecToString(prop, int64Vals);

        case (int32_t) VehiclePropertyType::FLOAT :
            if (val.float_values_size() > 0) {
                return floatToString(prop, val.float_values(0));
            } else {
                return "N/A";
            }

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
            return QObject::tr("Display N/A (bytes)");

        case (int32_t) VehiclePropertyType::MIXED :
            return QObject::tr("Display N/A (mixed)");

    }
    return QObject::tr("Unknown value type");
}

QString carpropertyutils::getAreaString(VehiclePropValue val) {
    int maskedProp = val.prop() & (int) VehicleArea::MASK;
    PropertyDescription prop = areaMap[maskedProp];
    QString area = prop.label;
    if (prop.lookupTableName != nullptr) {
        auto lookupTable = lookupTablesMap.find(prop.lookupTableName);
        if (lookupTable == lookupTablesMap.end()) {
            return area;
        }
        QString details = multiDetailToString(*(lookupTable->second), val.area_id());
        if (details != "") {
            area += ": " + details;
        }
    }
    return area;
}

QString carpropertyutils::booleanToString(PropertyDescription prop, int32_t val) {
    return val ? QObject::tr("True") : QObject::tr("False");
}

QString carpropertyutils::int32ToString(PropertyDescription prop, int32_t val) {
    if (prop.int32ToString != nullptr) {
        return prop.int32ToString(val);
    } else if (prop.lookupTableName != nullptr) {
        auto lookupTable = lookupTablesMap.find(prop.lookupTableName);
        if (lookupTable == lookupTablesMap.end()) {
            return QString::number(val);
        }
        return (*(lookupTable->second))[val];
    } else {
        return QString::number(val);
    }
}

QString carpropertyutils::int32VecToString(PropertyDescription prop, vector<int32_t> vals) {
    // Vector is either treated specially as a group, or is just a list
    // of distinct values.
    if (prop.int32VecToString != nullptr) {
        return prop.int32VecToString(vals);
    } else  {
        QString concat;
        for (int32_t val : vals) {
            concat += (int32ToString(prop, val) + "; ");
        }
        concat.chop(2);
        return concat;
    }
}

QString carpropertyutils::int64ToString(PropertyDescription prop, int64_t val) {
    // Currently no properties with this value type, so returning raw value.
    return QString::number(val);
}

QString carpropertyutils::int64VecToString(PropertyDescription prop, vector<int64_t> vals) {
    // Currently the only property with this type is too complex to show in a
    // small cell, so just returning the raw values.
    QString concat;
    for (int64_t val : vals) {
        concat += (int64ToString(prop, val) + "; ");
    }
    concat.chop(2);
    return concat;
}

QString carpropertyutils::floatToString(PropertyDescription prop, float val) {
    // Currently, the properties that use this value type only care about its
    // raw value, so that's what this returns.
    return QString::number(val);
}

QString carpropertyutils::floatVecToString(PropertyDescription prop, vector<float> vals) {
    // Currently no properties with this type, so just returning raw values.
    QString concat;
    for (float val : vals) {
        concat += (floatToString(prop, val) + "; ");
    }
    concat.chop(2);
    return concat;
}

QString carpropertyutils::int32ToHexString(int32_t val) {
    stringstream valStringStream;
    valStringStream << hex << val;
    return QObject::tr(valStringStream.str().c_str());
}

QString carpropertyutils::multiDetailToString(map<int, QString> lookupTable, int value) {
    QString details;
    for (const auto &locDetail : lookupTable) {
        if ((value & locDetail.first) == locDetail.first) {
            details += (locDetail.second + ", ");
        }
    }
    details.chop(2);
    return details;
}

QString carpropertyutils::fanDirectionToString(int val) {
    return multiDetailToString(fanDirectionMap, val);
}

QString carpropertyutils::heatingCoolingToString(int val) {
    QString behavior;
    if (val < 0) {
        behavior = QObject::tr("Cooling");
    } else if (val == 0) {
        behavior = QObject::tr("Off");
    } else {
        behavior = QObject::tr("On");
    }
    return QString::number(val) + " (" + behavior + ")";
}

QString carpropertyutils::apPowerStateReqToString(vector<int32_t> vals) {
    QString output = apPowerStateReqMap[vals[0]];
    if (vals[0] == (int) VehicleApPowerStateReq::SHUTDOWN_PREPARE) {
        output += apPowerStateShutdownMap[vals[1]];
    }
    return output;
}

QString carpropertyutils::apPowerStateReportToString(vector<int32_t> vals) {
    QString output = apPowerStateReportMap[vals[0]];
    if (vals[0] == (int) VehicleApPowerStateReport::SHUTDOWN_POSTPONE) {
        output += "Time: " + QString::number(vals[1]) + " ms";
    } else if (vals[0] == (int) VehicleApPowerStateReport::SHUTDOWN_START) {
        output += "Time to turn on AP: " + QString::number(vals[1]) + " s";
    }
    return QObject::tr(output.toLocal8Bit().data());
}

QString carpropertyutils::vendorIdToString(int32_t val){
    QString vendorLabel = "Vendor(id: 0x" + int32ToHexString(val) + ")";
    return QObject::tr(vendorLabel.toLocal8Bit().data());
}

QString carpropertyutils::changeModeToString(int32_t val) {
    QString changeMode;
    if (val == (int32_t)emulator::VehiclePropertyChangeMode::CONTINUOUS) {
        changeMode = QObject::tr("Continuous");
    } else if (val == (int32_t)emulator::VehiclePropertyChangeMode::ON_CHANGE) {
        changeMode = QObject::tr("On Change");
    } else if (val == (int32_t)emulator::VehiclePropertyChangeMode::STATIC) {
        changeMode = QObject::tr("Static");
    } else {
        changeMode = QObject::tr("Unknown");
    }
    return changeMode;
}

bool carpropertyutils::isVendor(int32_t val) {
    return (val & (int32_t)VehiclePropertyGroup::VENDOR)
            == (int32_t)VehiclePropertyGroup::VENDOR;
}

void carpropertyutils::loadDescriptionsFromJson(const char *filename) {
    QFile file(filename);
    LOG(DEBUG) << "Reading vendor vhal properties json file: " << filename << std::endl;
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
          LOG(WARNING) << "Can't open vhal properties file: " << filename << std::endl;
          return;
    }
    QByteArray jsonData = file.readAll();
    file.close();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    if (jsonDoc.isNull()) {
        LOG(WARNING) << "Can't read VHAL properties metadata: " << filename <<
            ". JSON is malformed!" << std::endl;
        return;
    }
    QJsonArray vehicleEnumsArray = jsonDoc.array();
    for (auto enumObject : vehicleEnumsArray) {
        if (enumObject.toObject().value("name").toString().compare("VehicleProperty") == 0) {
            // Add vehicle property description
            QJsonArray vehiclePropsArray = enumObject.toObject().value("values").toArray();
            for (auto vehiclePropObject : vehiclePropsArray) {
                const int propId = vehiclePropObject.toObject().value("value").toInt();
                if (propMap.find(propId) == propMap.end()) {
                    propMap[propId] =
                    { QObject::tr(vehiclePropObject.toObject().value("name").toString().toUtf8()
                        .constData()),
                      vehiclePropObject.toObject().value("data_enum").toString().toUtf8()
                        .constData() };
                }
            }
        } else {
            // Add enum values names
            QString name = enumObject.toObject().value("name").toString();
            if (lookupTablesMap.find(name) == lookupTablesMap.end()) {
                QJsonArray enumsArray = enumObject.toObject().value("values").toArray();
                auto enumsMap = new std::map<int32_t, QString>();
                for (auto enumObject : enumsArray) {
                    (*enumsMap)[enumObject.toObject().value("value").toInt()] =
                                                enumObject.toObject().value("name").toString();
                }
                lookupTablesMap[name] = enumsMap;
            }
        }
    }
}
