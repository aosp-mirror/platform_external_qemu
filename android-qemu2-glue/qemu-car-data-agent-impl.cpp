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

#include "android-qemu2-glue/qemu-control-impl.h"

#include "android/emulation/control/car_data_agent.h"
#include "android/car.h"
#include "android/utils/debug.h"
#include <functional>

static void send_car_data(const char* msg) {
    return android_send_car_data(msg);
}

static void set_car_data_call_back(const std::function<void(const char*)>& callback) {
    set_call_back(callback);
}

static const QCarDataAgent carDataAgent = {
    .sendCarData = send_car_data,
    .setCallback = set_car_data_call_back,
};

const QCarDataAgent* const gQCarDataAgent = &carDataAgent;

