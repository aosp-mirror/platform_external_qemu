// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/emulation/FakeCameraSensor.h"
#include "android/emulation/AndroidMessagePipe.h"
#include "android/utils/debug.h"
#include "android/hw-sensors.h"
//#include "keymaster_ipc.h"

#include <assert.h>

#include <memory>
#include <vector>

// #define DEBUG 1

#ifdef DEBUG
#include <stdio.h>
#define D(...)                                     \
    do {                                           \
        printf("%s:%d: ", __FUNCTION__, __LINE__); \
        printf(__VA_ARGS__);                       \
        fflush(stdout);                            \
    } while (0)
#else
#define D(...) ((void)0)
#endif

namespace android {

struct MyAcclMag {
    float ax;
    float ay;
    float az;
    float mx;
    float my;
    float mz;
    float ox;
    float oy;
    float oz;
};

void fakeCameraSensorDecodeAndExecute(const std::vector<uint8_t>& input,
                               std::vector<uint8_t>* outputp) {
    std::vector<uint8_t> & output = *outputp;
    //acceleration first: x y z
    //magnetic field second: x y z
    MyAcclMag v;
    android_sensors_get(ANDROID_SENSOR_ACCELERATION, &v.ax, &v.ay, &v.az);
    android_sensors_get(ANDROID_SENSOR_MAGNETIC_FIELD, &v.mx, &v.my, &v.mz);
    android_sensors_get(ANDROID_SENSOR_ORIENTATION, &v.ox, &v.oy, &v.oz);
    output.resize(sizeof(v));
    memcpy(&(output[0]), &v, sizeof(v));
}

void registerFakeCameraSensorServicePipe() {
    android::AndroidPipe::Service::add(new android::AndroidMessagePipe::Service(
            "FakeCameraSensor", fakeCameraSensorDecodeAndExecute));
}
}  // namespace android

extern "C" void android_init_fake_camera_sensor(void) {
    //keymaster_ipc_init();
    android::registerFakeCameraSensorServicePipe();
}
