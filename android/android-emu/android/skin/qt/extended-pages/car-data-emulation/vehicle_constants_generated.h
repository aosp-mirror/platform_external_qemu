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

//DO NOT EDIT, USE update_enum.sh TO UPDATE

#pragma once

#undef TRY_AGAIN

namespace emulator {
enum class VehiclePropertyType : int32_t;
enum class VehicleArea : int32_t;
enum class VehiclePropertyGroup : int32_t;
enum class VehicleProperty : int32_t;
enum class VehicleLightState : int32_t;
enum class VehicleLightSwitch : int32_t;
enum class EvConnectorType : int32_t;
enum class PortLocationType : int32_t;
enum class FuelType : int32_t;
enum class VehicleHvacFanDirection : int32_t;
enum class VehicleOilLevel : int32_t;
enum class VehicleApPowerStateConfigFlag : int32_t;
enum class VehicleApPowerStateReq : int32_t;
enum class VehicleApPowerStateReqIndex : int32_t;
enum class VehicleApPowerStateShutdownParam : int32_t;
enum class VehicleApPowerStateReport : int32_t;
enum class VehicleApPowerBootupReason : int32_t;
enum class VehicleHwKeyInputAction : int32_t;
enum class VehicleDisplay : int32_t;
enum class VehicleUnit : int32_t;
enum class VehiclePropertyChangeMode : int32_t;
enum class VehiclePropertyAccess : int32_t;
enum class VehiclePropertyStatus : int32_t;
enum class VehicleGear : int32_t;
enum class VehicleAreaSeat : int32_t;
enum class VehicleAreaWindow : int32_t;
enum class VehicleAreaDoor : int32_t;
enum class VehicleAreaMirror : int32_t;
enum class VehicleTurnSignal : int32_t;
struct VehicleAreaConfig;
struct VehiclePropConfig;
struct VehiclePropValue;
enum class VehicleIgnitionState : int32_t;
enum class SubscribeFlags : int32_t;
struct SubscribeOptions;
enum class StatusCode : int32_t;
enum class VehicleAreaWheel : int32_t;
enum class Obd2FuelSystemStatus : int32_t;
enum class Obd2IgnitionMonitorKind : int32_t;
enum class Obd2CommonIgnitionMonitors : int32_t;
enum class Obd2SparkIgnitionMonitors : int32_t;
enum class Obd2CompressionIgnitionMonitors : int32_t;
enum class Obd2SecondaryAirStatus : int32_t;
enum class Obd2FuelType : int32_t;
enum class DiagnosticIntegerSensorIndex : int32_t;
enum class DiagnosticFloatSensorIndex : int32_t;
enum class VmsMessageType : int32_t;
enum class VmsBaseMessageIntegerValuesIndex : int32_t;
enum class VmsMessageWithLayerIntegerValuesIndex : int32_t;
enum class VmsMessageWithLayerAndPublisherIdIntegerValuesIndex : int32_t;
enum class VmsOfferingMessageIntegerValuesIndex : int32_t;
enum class VmsSubscriptionsStateIntegerValuesIndex : int32_t;
enum class VmsAvailabilityStateIntegerValuesIndex : int32_t;
enum class VmsPublisherInformationIntegerValuesIndex : int32_t;

/**
 * Enumerates supported data type for VehicleProperty.
 * 
 * Used to create property ID in VehicleProperty enum.
 */
enum class VehiclePropertyType : int32_t {
    STRING = 1048576,
    BOOLEAN = 2097152,
    INT32 = 4194304,
    INT32_VEC = 4259840,
    INT64 = 5242880,
    INT64_VEC = 5308416,
    FLOAT = 6291456,
    FLOAT_VEC = 6356992,
    BYTES = 7340032,
    /**
     * Any combination of scalar or vector types. The exact format must be
     * provided in the description of the property.
     */
    MIXED = 14680064,
    MASK = 16711680,
};

enum class VehicleArea : int32_t {
    GLOBAL = 16777216,
    /**
     * WINDOW maps to enum VehicleAreaWindow  */
    WINDOW = 50331648,
    /**
     * MIRROR maps to enum VehicleAreaMirror  */
    MIRROR = 67108864,
    /**
     * SEAT maps to enum VehicleAreaSeat  */
    SEAT = 83886080,
    /**
     * DOOR maps to enum VehicleAreaDoor  */
    DOOR = 100663296,
    /**
     * WHEEL maps to enum VehicleAreaWheel  */
    WHEEL = 117440512,
    MASK = 251658240,
};

enum class VehiclePropertyGroup : int32_t {
    /**
     * Properties declared in AOSP must use this flag.
     */
    SYSTEM = 268435456,
    /**
     * Properties declared by vendors must use this flag.
     */
    VENDOR = 536870912,
    MASK = -268435456,
};

enum class VehicleProperty : int32_t {
    /**
     * Undefined property.  */
    INVALID = 0,
    /**
     * VIN of vehicle
     * 
     * @change_mode VehiclePropertyChangeMode:STATIC
     * @access VehiclePropertyAccess:READ
     */
    INFO_VIN = 286261504, // (((0x0100 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:STRING) | VehicleArea:GLOBAL)
    /**
     * Manufacturer of vehicle
     * 
     * @change_mode VehiclePropertyChangeMode:STATIC
     * @access VehiclePropertyAccess:READ
     */
    INFO_MAKE = 286261505, // (((0x0101 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:STRING) | VehicleArea:GLOBAL)
    /**
     * Model of vehicle
     * 
     * @change_mode VehiclePropertyChangeMode:STATIC
     * @access VehiclePropertyAccess:READ
     */
    INFO_MODEL = 286261506, // (((0x0102 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:STRING) | VehicleArea:GLOBAL)
    /**
     * Model year of vehicle.
     * 
     * @change_mode VehiclePropertyChangeMode:STATIC
     * @access VehiclePropertyAccess:READ
     * @unit VehicleUnit:YEAR
     */
    INFO_MODEL_YEAR = 289407235, // (((0x0103 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    /**
     * Fuel capacity of the vehicle in milliliters
     * 
     * @change_mode VehiclePropertyChangeMode:STATIC
     * @access VehiclePropertyAccess:READ
     * @unit VehicleUnit:MILLILITER
     */
    INFO_FUEL_CAPACITY = 291504388, // (((0x0104 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:FLOAT) | VehicleArea:GLOBAL)
    /**
     * List of fuels the vehicle may use
     * 
     * @change_mode VehiclePropertyChangeMode:STATIC
     * @access VehiclePropertyAccess:READ
     * @data_enum FuelType
     */
    INFO_FUEL_TYPE = 289472773, // (((0x0105 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32_VEC) | VehicleArea:GLOBAL)
    /**
     * Battery capacity of the vehicle, if EV or hybrid.  This is the nominal
     * battery capacity when the vehicle is new.
     * 
     * @change_mode VehiclePropertyChangeMode:STATIC
     * @access VehiclePropertyAccess:READ
     * @unit VehicleUnit:WH
     */
    INFO_EV_BATTERY_CAPACITY = 291504390, // (((0x0106 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:FLOAT) | VehicleArea:GLOBAL)
    /**
     * List of connectors this EV may use
     * 
     * @change_mode VehiclePropertyChangeMode:STATIC
     * @data_enum EvConnectorType
     * @access VehiclePropertyAccess:READ
     */
    INFO_EV_CONNECTOR_TYPE = 289472775, // (((0x0107 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32_VEC) | VehicleArea:GLOBAL)
    /**
     * Fuel door location
     * 
     * @change_mode VehiclePropertyChangeMode:STATIC
     * @data_enum FuelDoorLocationType
     * @access VehiclePropertyAccess:READ
     */
    INFO_FUEL_DOOR_LOCATION = 289407240, // (((0x0108 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    /**
     * EV port location
     * 
     * @change_mode VehiclePropertyChangeMode:STATIC
     * @access VehiclePropertyAccess:READ
     * @data_enum PortLocationType
     */
    INFO_EV_PORT_LOCATION = 289407241, // (((0x0109 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    /**
     * Driver's seat location
     * 
     * @change_mode VehiclePropertyChangeMode:STATIC
     * @data_enum VehicleAreaSeat
     * @access VehiclePropertyAccess:READ
     */
    INFO_DRIVER_SEAT = 356516106, // (((0x010A | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Current odometer value of the vehicle
     * 
     * @change_mode VehiclePropertyChangeMode:CONTINUOUS
     * @access VehiclePropertyAccess:READ
     * @unit VehicleUnit:KILOMETER
     */
    PERF_ODOMETER = 291504644, // (((0x0204 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:FLOAT) | VehicleArea:GLOBAL)
    /**
     * Speed of the vehicle
     * 
     * @change_mode VehiclePropertyChangeMode:CONTINUOUS
     * @access VehiclePropertyAccess:READ
     * @unit VehicleUnit:METER_PER_SEC
     */
    PERF_VEHICLE_SPEED = 291504647, // (((0x0207 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:FLOAT) | VehicleArea:GLOBAL)
    /**
     * Temperature of engine coolant
     * 
     * @change_mode VehiclePropertyChangeMode:CONTINUOUS
     * @access VehiclePropertyAccess:READ
     * @unit VehicleUnit:CELSIUS
     */
    ENGINE_COOLANT_TEMP = 291504897, // (((0x0301 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:FLOAT) | VehicleArea:GLOBAL)
    /**
     * Engine oil level
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ
     * @data_enum VehicleOilLevel
     */
    ENGINE_OIL_LEVEL = 289407747, // (((0x0303 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    /**
     * Temperature of engine oil
     * 
     * @change_mode VehiclePropertyChangeMode:CONTINUOUS
     * @access VehiclePropertyAccess:READ
     * @unit VehicleUnit:CELSIUS
     */
    ENGINE_OIL_TEMP = 291504900, // (((0x0304 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:FLOAT) | VehicleArea:GLOBAL)
    /**
     * Engine rpm
     * 
     * @change_mode VehiclePropertyChangeMode:CONTINUOUS
     * @access VehiclePropertyAccess:READ
     * @unit VehicleUnit:RPM
     */
    ENGINE_RPM = 291504901, // (((0x0305 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:FLOAT) | VehicleArea:GLOBAL)
    /**
     * Reports wheel ticks
     * 
     * The first element in the vector is a reset count.  A reset indicates
     * previous tick counts are not comparable with this and future ones.  Some
     * sort of discontinuity in tick counting has occurred.
     * 
     * The next four elements represent ticks for individual wheels in the
     * following order: front left, front right, rear right, rear left.  All
     * tick counts are cumulative.  Tick counts increment when the vehicle
     * moves forward, and decrement when vehicles moves in reverse.  The ticks
     * should be reset to 0 when the vehicle is started by the user.
     * 
     *  int64Values[0] = reset count
     *  int64Values[1] = front left ticks
     *  int64Values[2] = front right ticks
     *  int64Values[3] = rear right ticks
     *  int64Values[4] = rear left ticks
     * 
     * configArray is used to indicate the micrometers-per-wheel-tick value and
     * which wheels are supported.  configArray is set as follows:
     * 
     *  configArray[0], bits [0:3] = supported wheels.  Uses enum Wheel.
     *  configArray[1] = micrometers per front left wheel tick
     *  configArray[2] = micrometers per front right wheel tick
     *  configArray[3] = micrometers per rear right wheel tick
     *  configArray[4] = micrometers per rear left wheel tick
     * 
     * NOTE:  If a wheel is not supported, its value shall always be set to 0.
     * 
     * VehiclePropValue.timestamp must be correctly filled in.
     * 
     * @change_mode VehiclePropertyChangeMode:CONTINUOUS
     * @access VehiclePropertyAccess:READ
     */
    WHEEL_TICK = 290521862, // (((0x0306 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT64_VEC) | VehicleArea:GLOBAL)
    /**
     * Fuel remaining in the the vehicle, in milliliters
     * 
     * Value may not exceed INFO_FUEL_CAPACITY
     * 
     * @change_mode VehiclePropertyChangeMode:CONTINUOUS
     * @access VehiclePropertyAccess:READ
     * @unit VehicleUnit:MILLILITER
     */
    FUEL_LEVEL = 291504903, // (((0x0307 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:FLOAT) | VehicleArea:GLOBAL)
    /**
     * Fuel door open
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    FUEL_DOOR_OPEN = 287310600, // (((0x0308 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:GLOBAL)
    /**
     * EV battery level in WH, if EV or hybrid
     * 
     * Value may not exceed INFO_EV_BATTERY_CAPACITY
     * 
     * @change_mode VehiclePropertyChangeMode:CONTINUOUS
     * @access VehiclePropertyAccess:READ
     * @unit VehicleUnit:WH
     */
    EV_BATTERY_LEVEL = 291504905, // (((0x0309 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:FLOAT) | VehicleArea:GLOBAL)
    /**
     * EV charge port open
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    EV_CHARGE_PORT_OPEN = 287310602, // (((0x030A | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:GLOBAL)
    /**
     * EV charge port connected
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ
     */
    EV_CHARGE_PORT_CONNECTED = 287310603, // (((0x030B | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:GLOBAL)
    /**
     * EV instantaneous charge rate in milliwatts
     * 
     * Positive value indicates battery is being charged.
     * Negative value indicates battery being discharged.
     * 
     * @change_mode VehiclePropertyChangeMode:CONTINUOUS
     * @access VehiclePropertyAccess:READ
     * @unit VehicleUnit:MW
     */
    EV_BATTERY_INSTANTANEOUS_CHARGE_RATE = 291504908, // (((0x030C | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:FLOAT) | VehicleArea:GLOBAL)
    /**
     * Range remaining
     * 
     * Meters remaining of fuel and charge.  Range remaining shall account for
     * all energy sources in a vehicle.  For example, a hybrid car's range will
     * be the sum of the ranges based on fuel and battery.
     * 
     * @change_mode VehiclePropertyChangeMode:CONTINUOUS
     * @access VehiclePropertyAccess:READ_WRITE
     * @unit VehicleUnit:METER
     */
    RANGE_REMAINING = 291504904, // (((0x0308 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:FLOAT) | VehicleArea:GLOBAL)
    /**
     * Tire pressure
     * 
     * min/max value indicates tire pressure sensor range.  Each tire will have a separate min/max
     * value denoted by its areaConfig.areaId.
     * 
     * @change_mode VehiclePropertyChangeMode:CONTINUOUS
     * @access VehiclePropertyAccess:READ
     * @unit VehicleUnit:KILOPASCAL
     */
    TIRE_PRESSURE = 392168201, // (((0x0309 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:FLOAT) | VehicleArea:WHEEL)
    /**
     * Currently selected gear
     * 
     * This is the gear selected by the user.
     * 
     * Values in the config data must represent the list of supported gears
     * for this vehicle.  For example, config data for an automatic transmission
     * must contain {GEAR_NEUTRAL, GEAR_REVERSE, GEAR_PARK, GEAR_DRIVE,
     * GEAR_1, GEAR_2,...} and for manual transmission the list must be
     * {GEAR_NEUTRAL, GEAR_REVERSE, GEAR_1, GEAR_2,...}
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ
     * @data_enum VehicleGear
     */
    GEAR_SELECTION = 289408000, // (((0x0400 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    /**
     * Current gear. In non-manual case, selected gear may not
     * match the current gear. For example, if the selected gear is GEAR_DRIVE,
     * the current gear will be one of GEAR_1, GEAR_2 etc, which reflects
     * the actual gear the transmission is currently running in.
     * 
     * Values in the config data must represent the list of supported gears
     * for this vehicle.  For example, config data for an automatic transmission
     * must contain {GEAR_NEUTRAL, GEAR_REVERSE, GEAR_PARK, GEAR_1, GEAR_2,...}
     * and for manual transmission the list must be
     * {GEAR_NEUTRAL, GEAR_REVERSE, GEAR_1, GEAR_2,...}. This list need not be the
     * same as that of the supported gears reported in GEAR_SELECTION.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ
     * @data_enum VehicleGear
     */
    CURRENT_GEAR = 289408001, // (((0x0401 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    /**
     * Parking brake state.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    PARKING_BRAKE_ON = 287310850, // (((0x0402 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:GLOBAL)
    /**
     * Auto-apply parking brake.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    PARKING_BRAKE_AUTO_APPLY = 287310851, // (((0x0403 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:GLOBAL)
    /**
     * Warning for fuel low level.
     * 
     * This property corresponds to the low fuel warning on the dashboard.
     * Once FUEL_LEVEL_LOW is set, it should not be cleared until more fuel is
     * added to the vehicle.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ
     */
    FUEL_LEVEL_LOW = 287310853, // (((0x0405 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:GLOBAL)
    /**
     * Night mode
     * 
     * True indicates that night mode is currently enabled.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ
     */
    NIGHT_MODE = 287310855, // (((0x0407 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:GLOBAL)
    /**
     * State of the vehicles turn signals
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ
     * @data_enum VehicleTurnSignal
     */
    TURN_SIGNAL_STATE = 289408008, // (((0x0408 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    /**
     * Represents ignition state
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ
     * @data_enum VehicleIgnitionState
     */
    IGNITION_STATE = 289408009, // (((0x0409 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    /**
     * ABS is active
     * 
     * Set to true when ABS is active.  Reset to false when ABS is off.  This
     * property may be intermittently set (pulsing) based on the real-time
     * state of the ABS system.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ
     */
    ABS_ACTIVE = 287310858, // (((0x040A | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:GLOBAL)
    /**
     * Traction Control is active
     * 
     * Set to true when traction control (TC) is active.  Reset to false when
     * TC is off.  This property may be intermittently set (pulsing) based on
     * the real-time state of the TC system.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ
     */
    TRACTION_CONTROL_ACTIVE = 287310859, // (((0x040B | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:GLOBAL)
    /**
     * Fan speed setting
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    HVAC_FAN_SPEED = 356517120, // (((0x0500 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Fan direction setting
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     * @data_enum VehicleHvacFanDirection
     */
    HVAC_FAN_DIRECTION = 356517121, // (((0x0501 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * HVAC current temperature.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ
     * @unit VehicleUnit:CELSIUS
     */
    HVAC_TEMPERATURE_CURRENT = 358614274, // (((0x0502 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:FLOAT) | VehicleArea:SEAT)
    /**
     * HVAC, target temperature set.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     * @unit VehicleUnit:CELSIUS
     */
    HVAC_TEMPERATURE_SET = 358614275, // (((0x0503 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:FLOAT) | VehicleArea:SEAT)
    /**
     * On/off defrost for designated window
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    HVAC_DEFROSTER = 320865540, // (((0x0504 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:WINDOW)
    /**
     * On/off AC for designated areaId
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     * @config_flags Supported areaIds
     */
    HVAC_AC_ON = 354419973, // (((0x0505 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:SEAT)
    /**
     * On/off max AC
     * 
     * When MAX AC is on, the ECU may adjust the vent position, fan speed,
     * temperature, etc as necessary to cool the vehicle as quickly as possible.
     * Any parameters modified as a side effect of turning on/off the MAX AC
     * parameter shall generate onPropertyEvent() callbacks to the VHAL.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    HVAC_MAX_AC_ON = 354419974, // (((0x0506 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:SEAT)
    /**
     * On/off max defrost
     * 
     * When MAX DEFROST is on, the ECU may adjust the vent position, fan speed,
     * temperature, etc as necessary to defrost the windows as quickly as
     * possible.  Any parameters modified as a side effect of turning on/off
     * the MAX DEFROST parameter shall generate onPropertyEvent() callbacks to
     * the VHAL.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    HVAC_MAX_DEFROST_ON = 354419975, // (((0x0507 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:SEAT)
    /**
     * Recirculation on/off
     * 
     * Controls the supply of exterior air to the cabin.  Recirc “on” means the
     * majority of the airflow into the cabin is originating in the cabin.
     * Recirc “off” means the majority of the airflow into the cabin is coming
     * from outside the car.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    HVAC_RECIRC_ON = 354419976, // (((0x0508 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:SEAT)
    /**
     * Enable temperature coupling between areas.
     * 
     * The AreaIDs for HVAC_DUAL_ON property shall contain a combination of
     * HVAC_TEMPERATURE_SET AreaIDs that can be coupled together. If
     * HVAC_TEMPERATURE_SET is mapped to AreaIDs [a_1, a_2, ..., a_n], and if
     * HVAC_DUAL_ON can be enabled to couple a_i and a_j, then HVAC_DUAL_ON
     * property must be mapped to [a_i | a_j]. Further, if a_k and a_l can also
     * be coupled together separately then HVAC_DUAL_ON must be mapped to
     * [a_i | a_j, a_k | a_l].
     * 
     * Example: A car has two front seats (ROW_1_LEFT, ROW_1_RIGHT) and three
     *  back seats (ROW_2_LEFT, ROW_2_CENTER, ROW_2_RIGHT). There are two
     *  temperature control units -- driver side and passenger side -- which can
     *  be optionally synchronized. This may be expressed in the AreaIDs this way:
     *  - HVAC_TEMPERATURE_SET->[ROW_1_LEFT | ROW_2_LEFT, ROW_1_RIGHT | ROW_2_CENTER | ROW_2_RIGHT]
     *  - HVAC_DUAL_ON->[ROW_1_LEFT | ROW_2_LEFT | ROW_1_RIGHT | ROW_2_CENTER | ROW_2_RIGHT]
     * 
     * When the property is enabled, the ECU must synchronize the temperature
     * for the affected areas. Any parameters modified as a side effect
     * of turning on/off the DUAL_ON parameter shall generate
     * onPropertyEvent() callbacks to the VHAL. In addition, if setting
     * a temperature (i.e. driver's temperature) changes another temperature
     * (i.e. front passenger's temperature), then the appropriate
     * onPropertyEvent() callbacks must be generated.  If a user changes a
     * temperature that breaks the coupling (e.g. setting the passenger
     * temperature independently) then the VHAL must send the appropriate
     * onPropertyEvent() callbacks (i.e. HVAC_DUAL_ON = false,
     * HVAC_TEMPERATURE_SET[AreaID] = xxx, etc).
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    HVAC_DUAL_ON = 354419977, // (((0x0509 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:SEAT)
    /**
     * On/off automatic mode
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    HVAC_AUTO_ON = 354419978, // (((0x050A | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:SEAT)
    /**
     * Seat heating/cooling
     * 
     * Negative values indicate cooling.
     * 0 indicates off.
     * Positive values indicate heating.
     * 
     * Some vehicles may have multiple levels of heating and cooling. The
     * min/max range defines the allowable range and number of steps in each
     * direction.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    HVAC_SEAT_TEMPERATURE = 356517131, // (((0x050B | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Side Mirror Heat
     * 
     * Increasing values denote higher heating levels for side mirrors.
     * The Max value in the config data represents the highest heating level.
     * The Min value in the config data MUST be zero and indicates no heating.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    HVAC_SIDE_MIRROR_HEAT = 339739916, // (((0x050C | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:MIRROR)
    /**
     * Steering Wheel Heating/Cooling
     * 
     * Sets the amount of heating/cooling for the steering wheel
     * config data Min and Max MUST be set appropriately.
     * Positive value indicates heating.
     * Negative value indicates cooling.
     * 0 indicates temperature control is off.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    HVAC_STEERING_WHEEL_HEAT = 289408269, // (((0x050D | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    /**
     * Temperature units for display
     * 
     * Indicates whether the vehicle is displaying temperature to the user as
     * Celsius or Fahrenheit.
     * This parameter MAY be used for displaying any HVAC temperature in the system.
     * Values must be one of VehicleUnit::CELSIUS or VehicleUnit::FAHRENHEIT
     * Note that internally, all temperatures are represented in floating point Celsius.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    HVAC_TEMPERATURE_DISPLAY_UNITS = 289408270, // (((0x050E | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    /**
     * Actual fan speed
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ
     */
    HVAC_ACTUAL_FAN_SPEED_RPM = 356517135, // (((0x050F | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Represents global power state for HVAC. Setting this property to false
     * MAY mark some properties that control individual HVAC features/subsystems
     * to UNAVAILABLE state. Setting this property to true MAY mark some
     * properties that control individual HVAC features/subsystems to AVAILABLE
     * state (unless any/all of them are UNAVAILABLE on their own individual
     * merits).
     * 
     * [Definition] HvacPower_DependentProperties: Properties that need HVAC to be
     *   powered on in order to enable their functionality. For example, in some cars,
     *   in order to turn on the AC, HVAC must be powered on first.
     * 
     * HvacPower_DependentProperties list must be set in the
     * VehiclePropConfig.configArray. HvacPower_DependentProperties must only contain
     * properties that are associated with VehicleArea:SEAT. Properties that are not
     * associated with VehicleArea:SEAT, for example, HVAC_DEFROSTER, must never
     * depend on HVAC_POWER_ON property and must never be part of
     * HvacPower_DependentProperties list.
     * 
     * AreaID mapping for HVAC_POWER_ON property must contain all AreaIDs that
     * HvacPower_DependentProperties are mapped to.
     * 
     * Example 1: A car has two front seats (ROW_1_LEFT, ROW_1_RIGHT) and three back
     *  seats (ROW_2_LEFT, ROW_2_CENTER, ROW_2_RIGHT). If the HVAC features (AC,
     *  Temperature etc.) throughout the car are dependent on a single HVAC power
     *  controller then HVAC_POWER_ON must be mapped to
     *  [ROW_1_LEFT | ROW_1_RIGHT | ROW_2_LEFT | ROW_2_CENTER | ROW_2_RIGHT].
     * 
     * Example 2: A car has two seats in the front row (ROW_1_LEFT, ROW_1_RIGHT) and
     *   three seats in the second (ROW_2_LEFT, ROW_2_CENTER, ROW_2_RIGHT) and third
     *   rows (ROW_3_LEFT, ROW_3_CENTER, ROW_3_RIGHT). If the car has temperature
     *   controllers in the front row which can operate entirely independently of
     *   temperature controllers in the back of the vehicle, then HVAC_POWER_ON
     *   must be mapped to a two element array:
     *   - ROW_1_LEFT | ROW_1_RIGHT
     *   - ROW_2_LEFT | ROW_2_CENTER | ROW_2_RIGHT | ROW_3_LEFT | ROW_3_CENTER | ROW_3_RIGHT
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    HVAC_POWER_ON = 354419984, // (((0x0510 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:SEAT)
    /**
     * Fan Positions Available
     * 
     * This is a bit mask of fan positions available for the zone.  Each
     * available fan direction is denoted by a separate entry in the vector.  A
     * fan direction may have multiple bits from vehicle_hvac_fan_direction set.
     * For instance, a typical car may have the following fan positions:
     *   - FAN_DIRECTION_FACE (0x1)
     *   - FAN_DIRECTION_FLOOR (0x2)
     *   - FAN_DIRECTION_FACE | FAN_DIRECTION_FLOOR (0x3)
     *   - FAN_DIRECTION_DEFROST (0x4)
     *   - FAN_DIRECTION_FLOOR | FAN_DIRECTION_DEFROST (0x6)
     * 
     * @change_mode VehiclePropertyChangeMode:STATIC
     * @access VehiclePropertyAccess:READ
     */
    HVAC_FAN_DIRECTION_AVAILABLE = 356582673, // (((0x0511 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32_VEC) | VehicleArea:SEAT)
    /**
     * Automatic recirculation on/off
     * 
     * When automatic recirculation is ON, the HVAC system may automatically
     * switch to recirculation mode if the vehicle detects poor incoming air
     * quality.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    HVAC_AUTO_RECIRC_ON = 354419986, // (((0x0512 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:SEAT)
    /**
     * Seat ventilation
     * 
     * 0 indicates off.
     * Positive values indicates ventilation level.
     * 
     * Used by HVAC apps and Assistant to enable, change, or read state of seat
     * ventilation.  This is different than seating cooling. It can be on at the
     * same time as cooling, or not.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    HVAC_SEAT_VENTILATION = 356517139, // (((0x0513 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Outside temperature
     * 
     * @change_mode VehiclePropertyChangeMode:CONTINUOUS
     * @access VehiclePropertyAccess:READ
     * @unit VehicleUnit:CELSIUS
     */
    ENV_OUTSIDE_TEMPERATURE = 291505923, // (((0x0703 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:FLOAT) | VehicleArea:GLOBAL)
    /**
     * Property to control power state of application processor
     * 
     * It is assumed that AP's power state is controller by separate power
     * controller.
     * 
     * For configuration information, VehiclePropConfig.configArray can have bit flag combining
     * values in VehicleApPowerStateConfigFlag.
     * 
     *   int32Values[0] : VehicleApPowerStateReq enum value
     *   int32Values[1] : additional parameter relevant for each state,
     *                    0 if not used.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VEHICLE_PROP_ACCESS_READ
     */
    AP_POWER_STATE_REQ = 289475072, // (((0x0A00 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32_VEC) | VehicleArea:GLOBAL)
    /**
     * Property to report power state of application processor
     * 
     * It is assumed that AP's power state is controller by separate power
     * controller.
     * 
     *   int32Values[0] : VehicleApPowerStateReport enum value
     *   int32Values[1] : Time in ms to wake up, if necessary.  Otherwise 0.
     * 
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VEHICLE_PROP_ACCESS_WRITE
     */
    AP_POWER_STATE_REPORT = 289475073, // (((0x0A01 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32_VEC) | VehicleArea:GLOBAL)
    /**
     * Property to report bootup reason for the current power on. This is a
     * static property that will not change for the whole duration until power
     * off. For example, even if user presses power on button after automatic
     * power on with door unlock, bootup reason must stay with
     * VehicleApPowerBootupReason#USER_UNLOCK.
     * 
     * int32Values[0] must be VehicleApPowerBootupReason.
     * 
     * @change_mode VehiclePropertyChangeMode:STATIC
     * @access VehiclePropertyAccess:READ
     */
    AP_POWER_BOOTUP_REASON = 289409538, // (((0x0A02 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    /**
     * Property to represent brightness of the display. Some cars have single
     * control for the brightness of all displays and this property is to share
     * change in that control.
     * 
     * If this is writable, android side can set this value when user changes
     * display brightness from Settings. If this is read only, user may still
     * change display brightness from Settings, but that must not be reflected
     * to other displays.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    DISPLAY_BRIGHTNESS = 289409539, // (((0x0A03 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    /**
     * Property to feed H/W input events to android
     * 
     * int32Values[0] : action defined by VehicleHwKeyInputAction
     * int32Values[1] : key code, must use standard android key code
     * int32Values[2] : target display defined in VehicleDisplay. Events not
     *                  tied to specific display must be sent to
     *                  VehicleDisplay#MAIN.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ
     * @config_flags
     */
    HW_KEY_INPUT = 289475088, // (((0x0A10 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32_VEC) | VehicleArea:GLOBAL)
    /**
     * ************************************************************************
     * Most Car Cabin properties have both a POSition and MOVE parameter.  These
     * are used to control the various movements for seats, doors, and windows
     * in a vehicle.
     * 
     * A POS parameter allows the user to set the absolution position.  For
     * instance, for a door, 0 indicates fully closed and max value indicates
     * fully open.  Thus, a value halfway between min and max must indicate
     * the door is halfway open.
     * 
     * A MOVE parameter moves the device in a particular direction.  The sign
     * indicates direction, and the magnitude indicates speed (if multiple
     * speeds are available).  For a door, a move of -1 will close the door, and
     * a move of +1 will open it.  Once a door reaches the limit of open/close,
     * the door should automatically stop moving.  The user must NOT need to
     * send a MOVE(0) command to stop the door at the end of its range.
     * ************************************************************************
     * 
     * Door position
     * 
     * This is an integer in case a door may be set to a particular position.
     * Max value indicates fully open, min value (0) indicates fully closed.
     * 
     * Some vehicles (minivans) can open the door electronically.  Hence, the
     * ability to write this property.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    DOOR_POS = 373295872, // (((0x0B00 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:DOOR)
    /**
     * Door move
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    DOOR_MOVE = 373295873, // (((0x0B01 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:DOOR)
    /**
     * Door lock
     * 
     * 'true' indicates door is locked
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    DOOR_LOCK = 371198722, // (((0x0B02 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:DOOR)
    /**
     * Mirror Z Position
     * 
     * Positive value indicates tilt upwards, negative value is downwards
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    MIRROR_Z_POS = 339741504, // (((0x0B40 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:MIRROR)
    /**
     * Mirror Z Move
     * 
     * Positive value indicates tilt upwards, negative value is downwards
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    MIRROR_Z_MOVE = 339741505, // (((0x0B41 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:MIRROR)
    /**
     * Mirror Y Position
     * 
     * Positive value indicate tilt right, negative value is left
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    MIRROR_Y_POS = 339741506, // (((0x0B42 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:MIRROR)
    /**
     * Mirror Y Move
     * 
     * Positive value indicate tilt right, negative value is left
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    MIRROR_Y_MOVE = 339741507, // (((0x0B43 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:MIRROR)
    /**
     * Mirror Lock
     * 
     * True indicates mirror positions are locked and not changeable
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    MIRROR_LOCK = 287312708, // (((0x0B44 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:GLOBAL)
    /**
     * Mirror Fold
     * 
     * True indicates mirrors are folded
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    MIRROR_FOLD = 287312709, // (((0x0B45 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:GLOBAL)
    /**
     * Seat memory select
     * 
     * This parameter selects the memory preset to use to select the seat
     * position. The minValue is always 0, and the maxValue determines the
     * number of seat positions available.
     * 
     * For instance, if the driver's seat has 3 memory presets, the maxValue
     * will be 3. When the user wants to select a preset, the desired preset
     * number (1, 2, or 3) is set.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:WRITE
     */
    SEAT_MEMORY_SELECT = 356518784, // (((0x0B80 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Seat memory set
     * 
     * This setting allows the user to save the current seat position settings
     * into the selected preset slot.  The maxValue for each seat position
     * must match the maxValue for SEAT_MEMORY_SELECT.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:WRITE
     */
    SEAT_MEMORY_SET = 356518785, // (((0x0B81 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Seatbelt buckled
     * 
     * True indicates belt is buckled.
     * 
     * Write access indicates automatic seat buckling capabilities.  There are
     * no known cars at this time, but you never know...
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    SEAT_BELT_BUCKLED = 354421634, // (((0x0B82 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:SEAT)
    /**
     * Seatbelt height position
     * 
     * Adjusts the shoulder belt anchor point.
     * Max value indicates highest position
     * Min value indicates lowest position
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    SEAT_BELT_HEIGHT_POS = 356518787, // (((0x0B83 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Seatbelt height move
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    SEAT_BELT_HEIGHT_MOVE = 356518788, // (((0x0B84 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Seat fore/aft position
     * 
     * Sets the seat position forward (closer to steering wheel) and backwards.
     * Max value indicates closest to wheel, min value indicates most rearward
     * position.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    SEAT_FORE_AFT_POS = 356518789, // (((0x0B85 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Seat fore/aft move
     * 
     * Moves the seat position forward and aft.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    SEAT_FORE_AFT_MOVE = 356518790, // (((0x0B86 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Seat backrest angle 1 position
     * 
     * Backrest angle 1 is the actuator closest to the bottom of the seat.
     * Max value indicates angling forward towards the steering wheel.
     * Min value indicates full recline.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    SEAT_BACKREST_ANGLE_1_POS = 356518791, // (((0x0B87 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Seat backrest angle 1 move
     * 
     * Moves the backrest forward or recline.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    SEAT_BACKREST_ANGLE_1_MOVE = 356518792, // (((0x0B88 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Seat backrest angle 2 position
     * 
     * Backrest angle 2 is the next actuator up from the bottom of the seat.
     * Max value indicates angling forward towards the steering wheel.
     * Min value indicates full recline.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    SEAT_BACKREST_ANGLE_2_POS = 356518793, // (((0x0B89 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Seat backrest angle 2 move
     * 
     * Moves the backrest forward or recline.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    SEAT_BACKREST_ANGLE_2_MOVE = 356518794, // (((0x0B8A | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Seat height position
     * 
     * Sets the seat height.
     * Max value indicates highest position.
     * Min value indicates lowest position.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    SEAT_HEIGHT_POS = 356518795, // (((0x0B8B | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Seat height move
     * 
     * Moves the seat height.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    SEAT_HEIGHT_MOVE = 356518796, // (((0x0B8C | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Seat depth position
     * 
     * Sets the seat depth, distance from back rest to front edge of seat.
     * Max value indicates longest depth position.
     * Min value indicates shortest position.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    SEAT_DEPTH_POS = 356518797, // (((0x0B8D | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Seat depth move
     * 
     * Adjusts the seat depth.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    SEAT_DEPTH_MOVE = 356518798, // (((0x0B8E | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Seat tilt position
     * 
     * Sets the seat tilt.
     * Max value indicates front edge of seat higher than back edge.
     * Min value indicates front edge of seat lower than back edge.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    SEAT_TILT_POS = 356518799, // (((0x0B8F | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Seat tilt move
     * 
     * Tilts the seat.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    SEAT_TILT_MOVE = 356518800, // (((0x0B90 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Lumber fore/aft position
     * 
     * Pushes the lumbar support forward and backwards
     * Max value indicates most forward position.
     * Min value indicates most rearward position.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    SEAT_LUMBAR_FORE_AFT_POS = 356518801, // (((0x0B91 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Lumbar fore/aft move
     * 
     * Adjusts the lumbar support.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    SEAT_LUMBAR_FORE_AFT_MOVE = 356518802, // (((0x0B92 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Lumbar side support position
     * 
     * Sets the amount of lateral lumbar support.
     * Max value indicates widest lumbar setting (i.e. least support)
     * Min value indicates thinnest lumbar setting.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    SEAT_LUMBAR_SIDE_SUPPORT_POS = 356518803, // (((0x0B93 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Lumbar side support move
     * 
     * Adjusts the amount of lateral lumbar support.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    SEAT_LUMBAR_SIDE_SUPPORT_MOVE = 356518804, // (((0x0B94 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Headrest height position
     * 
     * Sets the headrest height.
     * Max value indicates tallest setting.
     * Min value indicates shortest setting.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    SEAT_HEADREST_HEIGHT_POS = 289409941, // (((0x0B95 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    /**
     * Headrest height move
     * 
     * Moves the headrest up and down.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    SEAT_HEADREST_HEIGHT_MOVE = 356518806, // (((0x0B96 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Headrest angle position
     * 
     * Sets the angle of the headrest.
     * Max value indicates most upright angle.
     * Min value indicates shallowest headrest angle.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    SEAT_HEADREST_ANGLE_POS = 356518807, // (((0x0B97 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Headrest angle move
     * 
     * Adjusts the angle of the headrest
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    SEAT_HEADREST_ANGLE_MOVE = 356518808, // (((0x0B98 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Headrest fore/aft position
     * 
     * Adjusts the headrest forwards and backwards.
     * Max value indicates position closest to front of car.
     * Min value indicates position closest to rear of car.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    SEAT_HEADREST_FORE_AFT_POS = 356518809, // (((0x0B99 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Headrest fore/aft move
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    SEAT_HEADREST_FORE_AFT_MOVE = 356518810, // (((0x0B9A | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    /**
     * Window Position
     * 
     * Min = window up / closed
     * Max = window down / open
     * 
     * For a window that may open out of plane (i.e. vent mode of sunroof) this
     * parameter will work with negative values as follows:
     *  Max = sunroof completely open
     *  0 = sunroof closed.
     *  Min = sunroof vent completely open
     * 
     *  Note that in this mode, 0 indicates the window is closed.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    WINDOW_POS = 322964416, // (((0x0BC0 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:WINDOW)
    /**
     * Window Move
     * 
     * Max = Open the window as fast as possible
     * Min = Close the window as fast as possible
     * Magnitude denotes relative speed.  I.e. +2 is faster than +1 in closing
     * the window.
     * 
     * For a window that may open out of plane (i.e. vent mode of sunroof) this
     * parameter will work as follows:
     * 
     * If sunroof is open:
     *   Max = open the sunroof further, automatically stop when fully open.
     *   Min = close the sunroof, automatically stop when sunroof is closed.
     * 
     * If vent is open:
     *   Max = close the vent, automatically stop when vent is closed.
     *   Min = open the vent further, automatically stop when vent is fully open.
     * 
     * If sunroof is in the closed position:
     *   Max = open the sunroof, automatically stop when sunroof is fully open.
     *   Min = open the vent, automatically stop when vent is fully open.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    WINDOW_MOVE = 322964417, // (((0x0BC1 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:WINDOW)
    /**
     * Window Lock
     * 
     * True indicates windows are locked and can't be moved.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    WINDOW_LOCK = 320867268, // (((0x0BC4 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:WINDOW)
    /**
     * Vehicle Maps Service (VMS) message
     * 
     * This property uses MIXED data to communicate vms messages.
     * 
     * Its contents are to be interpreted as follows:
     * the indices defined in VmsMessageIntegerValuesIndex are to be used to
     * read from int32Values;
     * bytes is a serialized VMS message as defined in the vms protocol
     * which is opaque to the framework;
     * 
     * IVehicle#get must always return StatusCode::NOT_AVAILABLE.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     */
    VEHICLE_MAP_SERVICE = 299895808, // (((0x0C00 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:MIXED) | VehicleArea:GLOBAL)
    /**
     * OBD2 Live Sensor Data
     * 
     * Reports a snapshot of the current (live) values of the OBD2 sensors available.
     * 
     * The configArray is set as follows:
     *   configArray[0] = number of vendor-specific integer-valued sensors
     *   configArray[1] = number of vendor-specific float-valued sensors
     * 
     * The values of this property are to be interpreted as in the following example.
     * Considering a configArray = {2,3}
     * int32Values must be a vector containing Obd2IntegerSensorIndex.LAST_SYSTEM_INDEX + 2
     * elements (that is, 33 elements);
     * floatValues must be a vector containing Obd2FloatSensorIndex.LAST_SYSTEM_INDEX + 3
     * elements (that is, 73 elements);
     * 
     * It is possible for each frame to contain a different subset of sensor values, both system
     * provided sensors, and vendor-specific ones. In order to support that, the bytes element
     * of the property value is used as a bitmask,.
     * 
     * bytes must have a sufficient number of bytes to represent the total number of possible
     * sensors (in this case, 14 bytes to represent 106 possible values); it is to be read as
     * a contiguous bitmask such that each bit indicates the presence or absence of a sensor
     * from the frame, starting with as many bits as the size of int32Values, immediately
     * followed by as many bits as the size of floatValues.
     * 
     * For example, should bytes[0] = 0x4C (0b01001100) it would mean that:
     *   int32Values[0 and 1] are not valid sensor values
     *   int32Values[2 and 3] are valid sensor values
     *   int32Values[4 and 5] are not valid sensor values
     *   int32Values[6] is a valid sensor value
     *   int32Values[7] is not a valid sensor value
     * Should bytes[5] = 0x61 (0b01100001) it would mean that:
     *   int32Values[32] is a valid sensor value
     *   floatValues[0 thru 3] are not valid sensor values
     *   floatValues[4 and 5] are valid sensor values
     *   floatValues[6] is not a valid sensor value
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ
     */
    OBD2_LIVE_FRAME = 299896064, // (((0x0D00 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:MIXED) | VehicleArea:GLOBAL)
    /**
     * OBD2 Freeze Frame Sensor Data
     * 
     * Reports a snapshot of the value of the OBD2 sensors available at the time that a fault
     * occurred and was detected.
     * 
     * A configArray must be provided with the same meaning as defined for OBD2_LIVE_FRAME.
     * 
     * The values of this property are to be interpreted in a similar fashion as those for
     * OBD2_LIVE_FRAME, with the exception that the stringValue field may contain a non-empty
     * diagnostic troubleshooting code (DTC).
     * 
     * A IVehicle#get request of this property must provide a value for int64Values[0].
     * This will be interpreted as the timestamp of the freeze frame to retrieve. A list of
     * timestamps can be obtained by a IVehicle#get of OBD2_FREEZE_FRAME_INFO.
     * 
     * Should no freeze frame be available at the given timestamp, a response of NOT_AVAILABLE
     * must be returned by the implementation. Because vehicles may have limited storage for
     * freeze frames, it is possible for a frame request to respond with NOT_AVAILABLE even if
     * the associated timestamp has been recently obtained via OBD2_FREEZE_FRAME_INFO.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ
     */
    OBD2_FREEZE_FRAME = 299896065, // (((0x0D01 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:MIXED) | VehicleArea:GLOBAL)
    /**
     * OBD2 Freeze Frame Information
     * 
     * This property describes the current freeze frames stored in vehicle
     * memory and available for retrieval via OBD2_FREEZE_FRAME.
     * 
     * The values are to be interpreted as follows:
     * each element of int64Values must be the timestamp at which a a fault code
     * has been detected and the corresponding freeze frame stored, and each
     * such element can be used as the key to OBD2_FREEZE_FRAME to retrieve
     * the corresponding freeze frame.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ
     */
    OBD2_FREEZE_FRAME_INFO = 299896066, // (((0x0D02 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:MIXED) | VehicleArea:GLOBAL)
    /**
     * OBD2 Freeze Frame Clear
     * 
     * This property allows deletion of any of the freeze frames stored in
     * vehicle memory, as described by OBD2_FREEZE_FRAME_INFO.
     * 
     * The configArray is set as follows:
     *  configArray[0] = 1 if the implementation is able to clear individual freeze frames
     *                   by timestamp, 0 otherwise
     * 
     * IVehicle#set of this property is to be interpreted as follows:
     *   if int64Values contains no elements, then all frames stored must be cleared;
     *   if int64Values contains one or more elements, then frames at the timestamps
     *   stored in int64Values must be cleared, and the others not cleared. Should the
     *   vehicle not support selective clearing of freeze frames, this latter mode must
     *   return NOT_AVAILABLE.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:WRITE
     */
    OBD2_FREEZE_FRAME_CLEAR = 299896067, // (((0x0D03 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:MIXED) | VehicleArea:GLOBAL)
    /**
     * Headlights State
     * 
     * Return the current state of headlights.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ
     * @data_enum VehicleLightState
     */
    HEADLIGHTS_STATE = 289410560, // (((0x0E00 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    /**
     * High beam lights state
     * 
     * Return the current state of high beam lights.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ
     * @data_enum VehicleLightState
     */
    HIGH_BEAM_LIGHTS_STATE = 289410561, // (((0x0E01 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    /**
     * Fog light state
     * 
     * Return the current state of fog lights.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ
     * @data_enum VehicleLightState
     */
    FOG_LIGHTS_STATE = 289410562, // (((0x0E02 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    /**
     * Hazard light status
     * 
     * Return the current status of hazard lights.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ
     * @data_enum VehicleLightState
     */
    HAZARD_LIGHTS_STATE = 289410563, // (((0x0E03 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    /**
     * Headlight switch
     * 
     * The setting that the user wants.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     * @data_enum VehicleLightSwitch
     */
    HEADLIGHTS_SWITCH = 289410576, // (((0x0E10 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    /**
     * High beam light switch
     * 
     * The setting that the user wants.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     * @data_enum VehicleLightSwitch
     */
    HIGH_BEAM_LIGHTS_SWITCH = 289410577, // (((0x0E11 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    /**
     * Fog light switch
     * 
     * The setting that the user wants.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     * @data_enum VehicleLightSwitch
     */
    FOG_LIGHTS_SWITCH = 289410578, // (((0x0E12 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    /**
     * Hazard light switch
     * 
     * The setting that the user wants.
     * 
     * @change_mode VehiclePropertyChangeMode:ON_CHANGE
     * @access VehiclePropertyAccess:READ_WRITE
     * @data_enum VehicleLightSwitch
     */
    HAZARD_LIGHTS_SWITCH = 289410579, // (((0x0E13 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
};

enum class VehicleLightState : int32_t {
    OFF = 0,
    ON = 1,
    DAYTIME_RUNNING = 2,
};

enum class VehicleLightSwitch : int32_t {
    OFF = 0,
    ON = 1,
    /**
     * Daytime running lights mode.  Most cars automatically use DRL but some
     * cars allow the user to activate them manually.
     */
    DAYTIME_RUNNING = 2,
    /**
     * Allows the vehicle ECU to set the lights automatically
     */
    AUTOMATIC = 256,
};

enum class EvConnectorType : int32_t {
    /**
     * Default type if the vehicle does not know or report the EV connector
     * type.
     */
    UNKNOWN = 0,
    IEC_TYPE_1_AC = 1,
    IEC_TYPE_2_AC = 2,
    IEC_TYPE_3_AC = 3,
    IEC_TYPE_4_DC = 4,
    IEC_TYPE_1_CCS_DC = 5,
    IEC_TYPE_2_CCS_DC = 6,
    TESLA_ROADSTER = 7,
    TESLA_HPWC = 8,
    TESLA_SUPERCHARGER = 9,
    GBT_AC = 10,
    GBT_DC = 11,
    /**
     * Connector type to use when no other types apply. Before using this
     * value, work with Google to see if the EvConnectorType enum can be
     * extended with an appropriate value.
     */
    OTHER = 101,
};

enum class PortLocationType : int32_t {
    /**
     * Default type if the vehicle does not know or report the Fuel door
     * and ev port location.
     */
    UNKNOWN = 0,
    FRONT_LEFT = 1,
    FRONT_RIGHT = 2,
    REAR_RIGHT = 3,
    REAR_LEFT = 4,
    FRONT = 5,
    REAR = 6,
};

enum class FuelType : int32_t {
    /**
     * Fuel type to use if the HU does not know on which types of fuel the vehicle
     * runs. The use of this value is generally discouraged outside of aftermarket units.
     */
    FUEL_TYPE_UNKNOWN = 0,
    /**
     * Unleaded gasoline  */
    FUEL_TYPE_UNLEADED = 1,
    /**
     * Leaded gasoline  */
    FUEL_TYPE_LEADED = 2,
    /**
     * Diesel #1  */
    FUEL_TYPE_DIESEL_1 = 3,
    /**
     * Diesel #2  */
    FUEL_TYPE_DIESEL_2 = 4,
    /**
     * Biodiesel  */
    FUEL_TYPE_BIODIESEL = 5,
    /**
     * 85% ethanol/gasoline blend  */
    FUEL_TYPE_E85 = 6,
    /**
     * Liquified petroleum gas  */
    FUEL_TYPE_LPG = 7,
    /**
     * Compressed natural gas  */
    FUEL_TYPE_CNG = 8,
    /**
     * Liquified natural gas  */
    FUEL_TYPE_LNG = 9,
    /**
     * Electric  */
    FUEL_TYPE_ELECTRIC = 10,
    /**
     * Hydrogen fuel cell  */
    FUEL_TYPE_HYDROGEN = 11,
    /**
     * Fuel type to use when no other types apply. Before using this value, work with
     * Google to see if the FuelType enum can be extended with an appropriate value.
     */
    FUEL_TYPE_OTHER = 12,
};

enum class VehicleHvacFanDirection : int32_t {
    FACE = 1,
    FLOOR = 2,
    DEFROST = 4,
};

enum class VehicleOilLevel : int32_t {
    /**
     * Oil level values
     */
    CRITICALLY_LOW = 0,
    LOW = 1,
    NORMAL = 2,
    HIGH = 3,
    ERROR = 4,
};

enum class VehicleApPowerStateConfigFlag : int32_t {
    /**
     * AP can enter deep sleep state. If not set, AP will always shutdown from
     * VehicleApPowerState#SHUTDOWN_PREPARE power state.
     */
    ENABLE_DEEP_SLEEP_FLAG = 1,
    /**
     * The power controller can power on AP from off state after timeout
     * specified in VehicleApPowerSet VEHICLE_AP_POWER_SET_SHUTDOWN_READY message.
     */
    CONFIG_SUPPORT_TIMER_POWER_ON_FLAG = 2,
};

enum class VehicleApPowerStateReq : int32_t {
    /**
     * vehicle HAL will never publish this state to AP  */
    OFF = 0,
    /**
     * vehicle HAL will never publish this state to AP  */
    DEEP_SLEEP = 1,
    /**
     * AP is on but display must be off.  */
    ON_DISP_OFF = 2,
    /**
     * AP is on with display on. This state allows full user interaction.  */
    ON_FULL = 3,
    /**
     * The power controller has requested AP to shutdown. AP can either enter
     * sleep state or start full shutdown. AP can also request postponing
     * shutdown by sending VehicleApPowerSetState#SHUTDOWN_POSTPONE message. The
     * power controller must change power state to this state to shutdown
     * system.
     * 
     * int32Values[1] : one of enum_vehicle_ap_power_state_shutdown_param_type
     */
    SHUTDOWN_PREPARE = 4,
};

enum class VehicleApPowerStateReqIndex : int32_t {
    STATE = 0,
    ADDITIONAL = 1,
};

enum class VehicleApPowerStateShutdownParam : int32_t {
    /**
     * AP must shutdown immediately. Postponing is not allowed.  */
    SHUTDOWN_IMMEDIATELY = 1,
    /**
     * AP can enter deep sleep instead of shutting down completely.  */
    CAN_SLEEP = 2,
    /**
     * AP can only shutdown with postponing allowed.  */
    SHUTDOWN_ONLY = 3,
};

enum class VehicleApPowerStateReport : int32_t {
    /**
     * AP has finished boot up, and can start shutdown if requested by power
     * controller.
     */
    BOOT_COMPLETE = 1,
    /**
     * AP is entering deep sleep state. How this state is implemented may vary
     * depending on each H/W, but AP's power must be kept in this state.
     */
    DEEP_SLEEP_ENTRY = 2,
    /**
     * AP is exiting from deep sleep state, and is in
     * VehicleApPowerState#SHUTDOWN_PREPARE state.
     * The power controller may change state to other ON states based on the
     * current state.
     */
    DEEP_SLEEP_EXIT = 3,
    /**
     * int32Values[1]: Time to postpone shutdown in ms. Maximum value can be
     *                 5000 ms.
     *                 If AP needs more time, it will send another POSTPONE
     *                 message before the previous one expires.
     */
    SHUTDOWN_POSTPONE = 4,
    /**
     * AP is starting shutting down. When system completes shutdown, everything
     * will stop in AP as kernel will stop all other contexts. It is
     * responsibility of vehicle HAL or lower level to synchronize that state
     * with external power controller. As an example, some kind of ping
     * with timeout in power controller can be a solution.
     * 
     * int32Values[1]: Time to turn on AP in secs. Power controller may turn on
     *                 AP after specified time so that AP can run tasks like
     *                 update. If it is set to 0, there is no wake up, and power
     *                 controller may not necessarily support wake-up. If power
     *                 controller turns on AP due to timer, it must start with
     *                 VehicleApPowerState#ON_DISP_OFF state, and after
     *                 receiving VehicleApPowerSetState#BOOT_COMPLETE, it shall
     *                 do state transition to
     *                 VehicleApPowerState#SHUTDOWN_PREPARE.
     */
    SHUTDOWN_START = 5,
    /**
     * User has requested to turn off headunit's display, which is detected in
     * android side.
     * The power controller may change the power state to
     * VehicleApPowerState#ON_DISP_OFF.
     */
    DISPLAY_OFF = 6,
    /**
     * User has requested to turn on headunit's display, most probably from power
     * key input which is attached to headunit. The power controller may change
     * the power state to VehicleApPowerState#ON_FULL.
     */
    DISPLAY_ON = 7,
};

enum class VehicleApPowerBootupReason : int32_t {
    /**
     * Power on due to user's pressing of power key or rotating of ignition
     * switch.
     */
    USER_POWER_ON = 0,
    /**
     * Automatic power on triggered by door unlock or any other kind of automatic
     * user detection.
     */
    USER_UNLOCK = 1,
    /**
     * Automatic power on triggered by timer. This only happens when AP has asked
     * wake-up after
     * certain time through time specified in
     * VehicleApPowerSetState#SHUTDOWN_START.
     */
    TIMER = 2,
};

enum class VehicleHwKeyInputAction : int32_t {
    /**
     * Key down  */
    ACTION_DOWN = 0,
    /**
     * Key up  */
    ACTION_UP = 1,
};

enum class VehicleDisplay : int32_t {
    /**
     * The primary Android display (for example, center console)  */
    MAIN = 0,
    INSTRUMENT_CLUSTER = 1,
};

enum class VehicleUnit : int32_t {
    SHOULD_NOT_USE = 0,
    METER_PER_SEC = 1,
    RPM = 2,
    HERTZ = 3,
    PERCENTILE = 16,
    MILLIMETER = 32,
    METER = 33,
    KILOMETER = 35,
    CELSIUS = 48,
    FAHRENHEIT = 49,
    KELVIN = 50,
    MILLILITER = 64,
    NANO_SECS = 80,
    SECS = 83,
    YEAR = 89,
    KILOPASCAL = 112,
    WATT_HOUR = 96,
    MILLIAMPERE = 97,
    MILLIVOLT = 98,
    MILLIWATTS = 99,
};

enum class VehiclePropertyChangeMode : int32_t {
    /**
     * Property of this type must never be changed. Subscription is not supported
     * for these properties.
     */
    STATIC = 0,
    /**
     * Properties of this type must report when there is a change.
     * IVehicle#get call must return the current value.
     * Set operation for this property is assumed to be asynchronous. When the
     * property is read (using IVehicle#get) after IVehicle#set, it may still
     * return old value until underlying H/W backing this property has actually
     * changed the state. Once state is changed, the property must dispatch
     * changed value as event.
     */
    ON_CHANGE = 1,
    /**
     * Properties of this type change continuously and require a fixed rate of
     * sampling to retrieve the data.  Implementers may choose to send extra
     * notifications on significant value changes.
     */
    CONTINUOUS = 2,
};

enum class VehiclePropertyAccess : int32_t {
    NONE = 0,
    READ = 1,
    WRITE = 2,
    READ_WRITE = 3,
};

enum class VehiclePropertyStatus : int32_t {
    /**
     * Property is available and behaving normally  */
    AVAILABLE = 0,
    /**
     * A property in this state is not available for reading and writing.  This
     * is a transient state that depends on the availability of the underlying
     * implementation (e.g. hardware or driver). It MUST NOT be used to
     * represent features that this vehicle is always incapable of.  A get() of
     * a property in this state MAY return an undefined value, but MUST
     * correctly describe its status as UNAVAILABLE A set() of a property in
     * this state MAY return NOT_AVAILABLE. The HAL implementation MUST ignore
     * the value of the status field when writing a property value coming from
     * Android.
     */
    UNAVAILABLE = 1,
    /**
     * There is an error with this property.  */
    ERROR = 2,
};

enum class VehicleGear : int32_t {
    GEAR_NEUTRAL = 1,
    GEAR_REVERSE = 2,
    GEAR_PARK = 4,
    GEAR_DRIVE = 8,
    GEAR_1 = 16,
    GEAR_2 = 32,
    GEAR_3 = 64,
    GEAR_4 = 128,
    GEAR_5 = 256,
    GEAR_6 = 512,
    GEAR_7 = 1024,
    GEAR_8 = 2048,
    GEAR_9 = 4096,
};

enum class VehicleAreaSeat : int32_t {
    ROW_1_LEFT = 1,
    ROW_1_CENTER = 2,
    ROW_1_RIGHT = 4,
    ROW_2_LEFT = 16,
    ROW_2_CENTER = 32,
    ROW_2_RIGHT = 64,
    ROW_3_LEFT = 256,
    ROW_3_CENTER = 512,
    ROW_3_RIGHT = 1024,
};

enum class VehicleAreaWindow : int32_t {
    FRONT_WINDSHIELD = 1,
    REAR_WINDSHIELD = 2,
    ROW_1_LEFT = 16,
    ROW_1_RIGHT = 64,
    ROW_2_LEFT = 256,
    ROW_2_RIGHT = 1024,
    ROW_3_LEFT = 4096,
    ROW_3_RIGHT = 16384,
    ROOF_TOP_1 = 65536,
    ROOF_TOP_2 = 131072,
};

enum class VehicleAreaDoor : int32_t {
    ROW_1_LEFT = 1,
    ROW_1_RIGHT = 4,
    ROW_2_LEFT = 16,
    ROW_2_RIGHT = 64,
    ROW_3_LEFT = 256,
    ROW_3_RIGHT = 1024,
    HOOD = 268435456,
    REAR = 536870912,
};

enum class VehicleAreaMirror : int32_t {
    DRIVER_LEFT = 1,
    DRIVER_RIGHT = 2,
    DRIVER_CENTER = 4,
};

enum class VehicleTurnSignal : int32_t {
    NONE = 0,
    RIGHT = 1,
    LEFT = 2,
};

enum class VehicleIgnitionState : int32_t {
    UNDEFINED = 0,
    /**
     * Steering wheel is locked  */
    LOCK = 1,
    /**
     * Steering wheel is not locked, engine and all accessories are OFF. If
     * car can be in LOCK and OFF state at the same time than HAL must report
     * LOCK state.
     */
    OFF = 2, // (::android::hardware::automotive::vehicle::V2_0::VehicleIgnitionState.LOCK implicitly + 1)
    /**
     * Typically in this state accessories become available (e.g. radio).
     * Instrument cluster and engine are turned off
     */
    ACC = 3, // (::android::hardware::automotive::vehicle::V2_0::VehicleIgnitionState.OFF implicitly + 1)
    /**
     * Ignition is in state ON. Accessories and instrument cluster available,
     * engine might be running or ready to be started.
     */
    ON = 4, // (::android::hardware::automotive::vehicle::V2_0::VehicleIgnitionState.ACC implicitly + 1)
    /**
     * Typically in this state engine is starting (cranking).  */
    START = 5, // (::android::hardware::automotive::vehicle::V2_0::VehicleIgnitionState.ON implicitly + 1)
};

enum class SubscribeFlags : int32_t {
    UNDEFINED = 0,
    /**
     * Subscribe to event that was originated in vehicle HAL
     * (most likely this event came from the vehicle itself).
     */
    EVENTS_FROM_CAR = 1,
    /**
     * Use this flag to subscribe on events when IVehicle#set(...) was called by
     * vehicle HAL's client (e.g. Car Service).
     */
    EVENTS_FROM_ANDROID = 2,
};

enum class StatusCode : int32_t {
    OK = 0,
    /**
     * Try again.  */
    TRY_AGAIN = 1,
    /**
     * Invalid argument provided.  */
    INVALID_ARG = 2,
    /**
     * This code must be returned when device that associated with the vehicle
     * property is not available. For example, when client tries to set HVAC
     * temperature when the whole HVAC unit is turned OFF.
     */
    NOT_AVAILABLE = 3,
    /**
     * Access denied  */
    ACCESS_DENIED = 4,
    /**
     * Something unexpected has happened in Vehicle HAL  */
    INTERNAL_ERROR = 5,
};

enum class VehicleAreaWheel : int32_t {
    UNKNOWN = 0,
    LEFT_FRONT = 1,
    RIGHT_FRONT = 2,
    LEFT_REAR = 4,
    RIGHT_REAR = 8,
};

enum class Obd2FuelSystemStatus : int32_t {
    OPEN_INSUFFICIENT_ENGINE_TEMPERATURE = 1,
    CLOSED_LOOP = 2,
    OPEN_ENGINE_LOAD_OR_DECELERATION = 4,
    OPEN_SYSTEM_FAILURE = 8,
    CLOSED_LOOP_BUT_FEEDBACK_FAULT = 16,
};

enum class Obd2IgnitionMonitorKind : int32_t {
    SPARK = 0,
    COMPRESSION = 1,
};

enum class Obd2CommonIgnitionMonitors : int32_t {
    COMPONENTS_AVAILABLE = 1, // (0x1 << 0)
    COMPONENTS_INCOMPLETE = 2, // (0x1 << 1)
    FUEL_SYSTEM_AVAILABLE = 4, // (0x1 << 2)
    FUEL_SYSTEM_INCOMPLETE = 8, // (0x1 << 3)
    MISFIRE_AVAILABLE = 16, // (0x1 << 4)
    MISFIRE_INCOMPLETE = 32, // (0x1 << 5)
};

enum class Obd2SparkIgnitionMonitors : int32_t {
    COMPONENTS_AVAILABLE = 1, // (0x1 << 0)
    COMPONENTS_INCOMPLETE = 2, // (0x1 << 1)
    FUEL_SYSTEM_AVAILABLE = 4, // (0x1 << 2)
    FUEL_SYSTEM_INCOMPLETE = 8, // (0x1 << 3)
    MISFIRE_AVAILABLE = 16, // (0x1 << 4)
    MISFIRE_INCOMPLETE = 32, // (0x1 << 5)
    EGR_AVAILABLE = 64, // (0x1 << 6)
    EGR_INCOMPLETE = 128, // (0x1 << 7)
    OXYGEN_SENSOR_HEATER_AVAILABLE = 256, // (0x1 << 8)
    OXYGEN_SENSOR_HEATER_INCOMPLETE = 512, // (0x1 << 9)
    OXYGEN_SENSOR_AVAILABLE = 1024, // (0x1 << 10)
    OXYGEN_SENSOR_INCOMPLETE = 2048, // (0x1 << 11)
    AC_REFRIGERANT_AVAILABLE = 4096, // (0x1 << 12)
    AC_REFRIGERANT_INCOMPLETE = 8192, // (0x1 << 13)
    SECONDARY_AIR_SYSTEM_AVAILABLE = 16384, // (0x1 << 14)
    SECONDARY_AIR_SYSTEM_INCOMPLETE = 32768, // (0x1 << 15)
    EVAPORATIVE_SYSTEM_AVAILABLE = 65536, // (0x1 << 16)
    EVAPORATIVE_SYSTEM_INCOMPLETE = 131072, // (0x1 << 17)
    HEATED_CATALYST_AVAILABLE = 262144, // (0x1 << 18)
    HEATED_CATALYST_INCOMPLETE = 524288, // (0x1 << 19)
    CATALYST_AVAILABLE = 1048576, // (0x1 << 20)
    CATALYST_INCOMPLETE = 2097152, // (0x1 << 21)
};

enum class Obd2CompressionIgnitionMonitors : int32_t {
    COMPONENTS_AVAILABLE = 1, // (0x1 << 0)
    COMPONENTS_INCOMPLETE = 2, // (0x1 << 1)
    FUEL_SYSTEM_AVAILABLE = 4, // (0x1 << 2)
    FUEL_SYSTEM_INCOMPLETE = 8, // (0x1 << 3)
    MISFIRE_AVAILABLE = 16, // (0x1 << 4)
    MISFIRE_INCOMPLETE = 32, // (0x1 << 5)
    EGR_OR_VVT_AVAILABLE = 64, // (0x1 << 6)
    EGR_OR_VVT_INCOMPLETE = 128, // (0x1 << 7)
    PM_FILTER_AVAILABLE = 256, // (0x1 << 8)
    PM_FILTER_INCOMPLETE = 512, // (0x1 << 9)
    EXHAUST_GAS_SENSOR_AVAILABLE = 1024, // (0x1 << 10)
    EXHAUST_GAS_SENSOR_INCOMPLETE = 2048, // (0x1 << 11)
    BOOST_PRESSURE_AVAILABLE = 4096, // (0x1 << 12)
    BOOST_PRESSURE_INCOMPLETE = 8192, // (0x1 << 13)
    NOx_SCR_AVAILABLE = 16384, // (0x1 << 14)
    NOx_SCR_INCOMPLETE = 32768, // (0x1 << 15)
    NMHC_CATALYST_AVAILABLE = 65536, // (0x1 << 16)
    NMHC_CATALYST_INCOMPLETE = 131072, // (0x1 << 17)
};

enum class Obd2SecondaryAirStatus : int32_t {
    UPSTREAM = 1,
    DOWNSTREAM_OF_CATALYCIC_CONVERTER = 2,
    FROM_OUTSIDE_OR_OFF = 4,
    PUMP_ON_FOR_DIAGNOSTICS = 8,
};

enum class Obd2FuelType : int32_t {
    NOT_AVAILABLE = 0,
    GASOLINE = 1,
    METHANOL = 2,
    ETHANOL = 3,
    DIESEL = 4,
    LPG = 5,
    CNG = 6,
    PROPANE = 7,
    ELECTRIC = 8,
    BIFUEL_RUNNING_GASOLINE = 9,
    BIFUEL_RUNNING_METHANOL = 10,
    BIFUEL_RUNNING_ETHANOL = 11,
    BIFUEL_RUNNING_LPG = 12,
    BIFUEL_RUNNING_CNG = 13,
    BIFUEL_RUNNING_PROPANE = 14,
    BIFUEL_RUNNING_ELECTRIC = 15,
    BIFUEL_RUNNING_ELECTRIC_AND_COMBUSTION = 16,
    HYBRID_GASOLINE = 17,
    HYBRID_ETHANOL = 18,
    HYBRID_DIESEL = 19,
    HYBRID_ELECTRIC = 20,
    HYBRID_RUNNING_ELECTRIC_AND_COMBUSTION = 21,
    HYBRID_REGENERATIVE = 22,
    BIFUEL_RUNNING_DIESEL = 23,
};

enum class DiagnosticIntegerSensorIndex : int32_t {
    /**
     * refer to FuelSystemStatus for a description of this value.  */
    FUEL_SYSTEM_STATUS = 0,
    MALFUNCTION_INDICATOR_LIGHT_ON = 1,
    /**
     * refer to IgnitionMonitorKind for a description of this value.  */
    IGNITION_MONITORS_SUPPORTED = 2,
    /**
     * The value of this sensor is a bitmask that specifies whether ignition-specific
     * tests are available and whether they are complete. The semantics of the individual
     * bits in this value are given by, respectively, SparkIgnitionMonitors and
     * CompressionIgnitionMonitors depending on the value of IGNITION_MONITORS_SUPPORTED.
     */
    IGNITION_SPECIFIC_MONITORS = 3,
    INTAKE_AIR_TEMPERATURE = 4,
    /**
     * refer to SecondaryAirStatus for a description of this value.  */
    COMMANDED_SECONDARY_AIR_STATUS = 5,
    NUM_OXYGEN_SENSORS_PRESENT = 6,
    RUNTIME_SINCE_ENGINE_START = 7,
    DISTANCE_TRAVELED_WITH_MALFUNCTION_INDICATOR_LIGHT_ON = 8,
    WARMUPS_SINCE_CODES_CLEARED = 9,
    DISTANCE_TRAVELED_SINCE_CODES_CLEARED = 10,
    ABSOLUTE_BAROMETRIC_PRESSURE = 11,
    CONTROL_MODULE_VOLTAGE = 12,
    AMBIENT_AIR_TEMPERATURE = 13,
    TIME_WITH_MALFUNCTION_LIGHT_ON = 14,
    TIME_SINCE_TROUBLE_CODES_CLEARED = 15,
    MAX_FUEL_AIR_EQUIVALENCE_RATIO = 16,
    MAX_OXYGEN_SENSOR_VOLTAGE = 17,
    MAX_OXYGEN_SENSOR_CURRENT = 18,
    MAX_INTAKE_MANIFOLD_ABSOLUTE_PRESSURE = 19,
    MAX_AIR_FLOW_RATE_FROM_MASS_AIR_FLOW_SENSOR = 20,
    /**
     * refer to FuelType for a description of this value.  */
    FUEL_TYPE = 21,
    FUEL_RAIL_ABSOLUTE_PRESSURE = 22,
    ENGINE_OIL_TEMPERATURE = 23,
    DRIVER_DEMAND_PERCENT_TORQUE = 24,
    ENGINE_ACTUAL_PERCENT_TORQUE = 25,
    ENGINE_REFERENCE_PERCENT_TORQUE = 26,
    ENGINE_PERCENT_TORQUE_DATA_IDLE = 27,
    ENGINE_PERCENT_TORQUE_DATA_POINT1 = 28,
    ENGINE_PERCENT_TORQUE_DATA_POINT2 = 29,
    ENGINE_PERCENT_TORQUE_DATA_POINT3 = 30,
    ENGINE_PERCENT_TORQUE_DATA_POINT4 = 31,
    LAST_SYSTEM_INDEX = 31, // ENGINE_PERCENT_TORQUE_DATA_POINT4
};

enum class DiagnosticFloatSensorIndex : int32_t {
    CALCULATED_ENGINE_LOAD = 0,
    ENGINE_COOLANT_TEMPERATURE = 1,
    SHORT_TERM_FUEL_TRIM_BANK1 = 2,
    LONG_TERM_FUEL_TRIM_BANK1 = 3,
    SHORT_TERM_FUEL_TRIM_BANK2 = 4,
    LONG_TERM_FUEL_TRIM_BANK2 = 5,
    FUEL_PRESSURE = 6,
    INTAKE_MANIFOLD_ABSOLUTE_PRESSURE = 7,
    ENGINE_RPM = 8,
    VEHICLE_SPEED = 9,
    TIMING_ADVANCE = 10,
    MAF_AIR_FLOW_RATE = 11,
    THROTTLE_POSITION = 12,
    OXYGEN_SENSOR1_VOLTAGE = 13,
    OXYGEN_SENSOR1_SHORT_TERM_FUEL_TRIM = 14,
    OXYGEN_SENSOR1_FUEL_AIR_EQUIVALENCE_RATIO = 15,
    OXYGEN_SENSOR2_VOLTAGE = 16,
    OXYGEN_SENSOR2_SHORT_TERM_FUEL_TRIM = 17,
    OXYGEN_SENSOR2_FUEL_AIR_EQUIVALENCE_RATIO = 18,
    OXYGEN_SENSOR3_VOLTAGE = 19,
    OXYGEN_SENSOR3_SHORT_TERM_FUEL_TRIM = 20,
    OXYGEN_SENSOR3_FUEL_AIR_EQUIVALENCE_RATIO = 21,
    OXYGEN_SENSOR4_VOLTAGE = 22,
    OXYGEN_SENSOR4_SHORT_TERM_FUEL_TRIM = 23,
    OXYGEN_SENSOR4_FUEL_AIR_EQUIVALENCE_RATIO = 24,
    OXYGEN_SENSOR5_VOLTAGE = 25,
    OXYGEN_SENSOR5_SHORT_TERM_FUEL_TRIM = 26,
    OXYGEN_SENSOR5_FUEL_AIR_EQUIVALENCE_RATIO = 27,
    OXYGEN_SENSOR6_VOLTAGE = 28,
    OXYGEN_SENSOR6_SHORT_TERM_FUEL_TRIM = 29,
    OXYGEN_SENSOR6_FUEL_AIR_EQUIVALENCE_RATIO = 30,
    OXYGEN_SENSOR7_VOLTAGE = 31,
    OXYGEN_SENSOR7_SHORT_TERM_FUEL_TRIM = 32,
    OXYGEN_SENSOR7_FUEL_AIR_EQUIVALENCE_RATIO = 33,
    OXYGEN_SENSOR8_VOLTAGE = 34,
    OXYGEN_SENSOR8_SHORT_TERM_FUEL_TRIM = 35,
    OXYGEN_SENSOR8_FUEL_AIR_EQUIVALENCE_RATIO = 36,
    FUEL_RAIL_PRESSURE = 37,
    FUEL_RAIL_GAUGE_PRESSURE = 38,
    COMMANDED_EXHAUST_GAS_RECIRCULATION = 39,
    EXHAUST_GAS_RECIRCULATION_ERROR = 40,
    COMMANDED_EVAPORATIVE_PURGE = 41,
    FUEL_TANK_LEVEL_INPUT = 42,
    EVAPORATION_SYSTEM_VAPOR_PRESSURE = 43,
    CATALYST_TEMPERATURE_BANK1_SENSOR1 = 44,
    CATALYST_TEMPERATURE_BANK2_SENSOR1 = 45,
    CATALYST_TEMPERATURE_BANK1_SENSOR2 = 46,
    CATALYST_TEMPERATURE_BANK2_SENSOR2 = 47,
    ABSOLUTE_LOAD_VALUE = 48,
    FUEL_AIR_COMMANDED_EQUIVALENCE_RATIO = 49,
    RELATIVE_THROTTLE_POSITION = 50,
    ABSOLUTE_THROTTLE_POSITION_B = 51,
    ABSOLUTE_THROTTLE_POSITION_C = 52,
    ACCELERATOR_PEDAL_POSITION_D = 53,
    ACCELERATOR_PEDAL_POSITION_E = 54,
    ACCELERATOR_PEDAL_POSITION_F = 55,
    COMMANDED_THROTTLE_ACTUATOR = 56,
    ETHANOL_FUEL_PERCENTAGE = 57,
    ABSOLUTE_EVAPORATION_SYSTEM_VAPOR_PRESSURE = 58,
    SHORT_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK1 = 59,
    SHORT_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK2 = 60,
    SHORT_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK3 = 61,
    SHORT_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK4 = 62,
    LONG_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK1 = 63,
    LONG_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK2 = 64,
    LONG_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK3 = 65,
    LONG_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK4 = 66,
    RELATIVE_ACCELERATOR_PEDAL_POSITION = 67,
    HYBRID_BATTERY_PACK_REMAINING_LIFE = 68,
    FUEL_INJECTION_TIMING = 69,
    ENGINE_FUEL_RATE = 70,
    LAST_SYSTEM_INDEX = 70, // ENGINE_FUEL_RATE
};

enum class VmsMessageType : int32_t {
    /**
     * A request from the subscribers to the VMS service to subscribe to a layer.
     * 
     * This message type uses enum VmsMessageWithLayerIntegerValuesIndex.
     */
    SUBSCRIBE = 1,
    /**
     * A request from the subscribers to the VMS service to subscribe to a layer from a specific publisher.
     * 
     * This message type uses enum VmsMessageWithLayerAndPublisherIdIntegerValuesIndex.
     */
    SUBSCRIBE_TO_PUBLISHER = 2,
    /**
     * A request from the subscribers to the VMS service to unsubscribes from a layer.
     * 
     * This message type uses enum VmsMessageWithLayerIntegerValuesIndex.
     */
    UNSUBSCRIBE = 3,
    /**
     * A request from the subscribers to the VMS service to unsubscribes from a layer from a specific publisher.
     * 
     * This message type uses enum VmsMessageWithLayerAndPublisherIdIntegerValuesIndex.
     */
    UNSUBSCRIBE_TO_PUBLISHER = 4,
    /**
     * Information from the publishers to the VMS service about the layers which the client can publish.
     * 
     * This message type uses enum VmsOfferingMessageIntegerValuesIndex.
     */
    OFFERING = 5,
    /**
     * A request from the subscribers to the VMS service to get the available layers.
     * 
     * This message type uses enum VmsBaseMessageIntegerValuesIndex.
     */
    AVAILABILITY_REQUEST = 6,
    /**
     * A request from the publishers to the VMS service to get the layers with subscribers.
     * 
     * This message type uses enum VmsBaseMessageIntegerValuesIndex.
     */
    SUBSCRIPTIONS_REQUEST = 7,
    /**
     * A response from the VMS service to the subscribers to a VmsMessageType.AVAILABILITY_REQUEST
     * 
     * This message type uses enum VmsAvailabilityStateIntegerValuesIndex.
     */
    AVAILABILITY_RESPONSE = 8,
    /**
     * A notification from the VMS service to the subscribers on a change in the available layers.
     * 
     * This message type uses enum VmsAvailabilityStateIntegerValuesIndex.
     */
    AVAILABILITY_CHANGE = 9,
    /**
     * A response from the VMS service to the publishers to a VmsMessageType.SUBSCRIPTIONS_REQUEST
     * 
     * This message type uses enum VmsSubscriptionsStateIntegerValuesIndex.
     */
    SUBSCRIPTIONS_RESPONSE = 10,
    /**
     * A notification from the VMS service to the publishers on a change in the layers with subscribers.
     * 
     * This message type uses enum VmsSubscriptionsStateIntegerValuesIndex.
     */
    SUBSCRIPTIONS_CHANGE = 11,
    /**
     * A message from the VMS service to the subscribers or from the publishers to the VMS service
     * with a serialized VMS data packet as defined in the VMS protocol.
     * 
     * This message type uses enum VmsMessageWithLayerAndPublisherIdIntegerValuesIndex.
     */
    DATA = 12,
    /**
     * A request from the publishers to the VMS service to get a Publisher ID for a serialized VMS
     * provider description packet as defined in the VMS protocol.
     * 
     * This message type uses enum VmsBaseMessageIntegerValuesIndex.
     */
    PUBLISHER_ID_REQUEST = 13,
    /**
     * A response from the VMS service to the publisher that contains a provider description packet
     * and the publisher ID assigned to it.
     * 
     * This message type uses enum VmsPublisherInformationIntegerValuesIndex.
     */
    PUBLISHER_ID_RESPONSE = 14,
    /**
     * A request from the subscribers to the VMS service to get information for a Publisher ID.
     * 
     * This message type uses enum VmsPublisherInformationIntegerValuesIndex.
     */
    PUBLISHER_INFORMATION_REQUEST = 15,
    /**
     * A response from the VMS service to the subscribers that contains a provider description packet
     * and the publisher ID assigned to it.
     * 
     * This message type uses enum VmsPublisherInformationIntegerValuesIndex.
     */
    PUBLISHER_INFORMATION_RESPONSE = 16,
    LAST_VMS_MESSAGE_TYPE = 16, // PUBLISHER_INFORMATION_RESPONSE
};

enum class VmsBaseMessageIntegerValuesIndex : int32_t {
    MESSAGE_TYPE = 0,
};

enum class VmsMessageWithLayerIntegerValuesIndex : int32_t {
    MESSAGE_TYPE = 0,
    LAYER_TYPE = 1,
    LAYER_SUBTYPE = 2,
    LAYER_VERSION = 3,
};

enum class VmsMessageWithLayerAndPublisherIdIntegerValuesIndex : int32_t {
    MESSAGE_TYPE = 0,
    LAYER_TYPE = 1,
    LAYER_SUBTYPE = 2,
    LAYER_VERSION = 3,
    PUBLISHER_ID = 4,
};

enum class VmsOfferingMessageIntegerValuesIndex : int32_t {
    MESSAGE_TYPE = 0,
    PUBLISHER_ID = 1,
    NUMBER_OF_OFFERS = 2,
    OFFERING_START = 3,
};

enum class VmsSubscriptionsStateIntegerValuesIndex : int32_t {
    MESSAGE_TYPE = 0,
    SEQUENCE_NUMBER = 1,
    NUMBER_OF_LAYERS = 2,
    NUMBER_OF_ASSOCIATED_LAYERS = 3,
    SUBSCRIPTIONS_START = 4,
};

enum class VmsAvailabilityStateIntegerValuesIndex : int32_t {
    MESSAGE_TYPE = 0,
    SEQUENCE_NUMBER = 1,
    NUMBER_OF_ASSOCIATED_LAYERS = 2,
    LAYERS_START = 3,
};

enum class VmsPublisherInformationIntegerValuesIndex : int32_t {
    MESSAGE_TYPE = 0,
    PUBLISHER_ID = 1,
};

} // namespace emulator
