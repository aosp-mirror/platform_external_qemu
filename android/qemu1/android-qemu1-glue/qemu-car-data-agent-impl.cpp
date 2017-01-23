// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android-qemu1-glue/qemu-control-impl.h"

#include "android/emulation/control/car_data_agent.h"
extern "C" {
    #include "android/car.h"
}

void send_car_data(const char* msg) {
    return android_send_car_data(msg);
}

static const QCarDataAgent carDataAgent = {
    .sendCarData = send_car_data
};

extern "C" const QCarDataAgent* const gQCarDataAgent = &carDataAgent;