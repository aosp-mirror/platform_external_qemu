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

#include "android/emulation/FakeRotatingCameraSensor.h"
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

void fakeCameraSensorDecodeAndExecute(const std::vector<uint8_t>& input,
                               std::vector<uint8_t>* outputp) {
    std::vector<uint8_t> & output = *outputp;
    output.resize(9*sizeof(float));
    float *p = reinterpret_cast<float*>(&(output[0]));
    android_sensors_get(ANDROID_SENSOR_ACCELERATION, p, p+1, p+2);
    android_sensors_get(ANDROID_SENSOR_MAGNETIC_FIELD, p+3, p+4, p+5);
    android_sensors_get(ANDROID_SENSOR_ORIENTATION, p+6, p+7, p+8);
}

void registerFakeRotatingCameraSensorServicePipe() {
    android::AndroidPipe::Service::add(new android::AndroidMessagePipe::Service(
            "FakeRotatingCameraSensor", fakeCameraSensorDecodeAndExecute));
}
}  // namespace android

extern "C" void android_init_fake_camera_sensor(void) {
    //keymaster_ipc_init();
    android::registerFakeRotatingCameraSensorServicePipe();
}
