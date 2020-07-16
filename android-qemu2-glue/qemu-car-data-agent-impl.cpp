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
#include "android/emulation/control/car_data_agent.h"

void send_car_data(const char* msg, int length) {
    android_send_car_data(msg, length);
}

void set_car_data_call_back(car_callback_t callback, void* context) {
    set_car_call_back(callback, context);
}

static const QCarDataAgent carDataAgent = {
        .setCarCallback = set_car_data_call_back,
        .sendCarData = send_car_data,
};

extern "C" const QCarDataAgent* const gQCarDataAgent = &carDataAgent;
