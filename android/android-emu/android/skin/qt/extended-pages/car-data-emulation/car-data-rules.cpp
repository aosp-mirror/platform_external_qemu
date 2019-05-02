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

#include "android/emulation/proto/VehicleHalProto.pb.h"
#include "android/skin/qt/extended-pages/car-data-emulation/car-data-rules.h"
#include "android/skin/qt/extended-pages/car-data-emulation/vehicle_constants_generated.h"


bool isActionAvaliable(int32_t prop)
{
   switch(prop) {
        case static_cast<int32_t>(emulator::VehicleProperty::PERF_VEHICLE_SPEED) :
            return isSpeedAvaliable();
        case static_cast<int32_t>(emulator::VehicleProperty::IGNITION_STATE) :
            return isIgnitionStateAvaliable();
        case static_cast<int32_t>(emulator::VehicleProperty::GEAR_SELECTION) :
            return isGearAvaliable();
        case static_cast<int32_t>(emulator::VehicleProperty::PARKING_BRAKE_ON) :
            return isParkingBreakAvaliable();
        default: 
            return true;
   }
}

bool isSpeedAvaliable(){
    return false;
}

bool isGearAvaliable(){
    return true;
}

bool isIgnitionStateAvaliable(){
    return false;
}

bool isParkingBreakAvaliable(){
    return true;
}
