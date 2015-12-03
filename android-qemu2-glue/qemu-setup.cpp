// Copyright 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android-qemu2-glue/qemu-setup.h"

#include "android/android.h"
#include "android-qemu2-glue/qemu-control-impl.h"

extern "C" void qemu2_android_console_setup( const QAndroidBatteryAgent* battery_agent,
        const QAndroidFingerAgent* finger_agent,
        const QAndroidLocationAgent* location_agent,
        const QAndroidUserEventAgent* user_event_agent,
        const QAndroidVmOperations* vm_operations,
        const QAndroidNetAgent* net_agent);

void qemu_android_emulation_setup() {
    android_emulation_setup(
            gQAndroidBatteryAgent,
            gQAndroidCellularAgent,
            gQAndroidFingerAgent,
            gQAndroidLocationAgent,
            gQAndroidSensorsAgent,
            gQAndroidTelephonyAgent,
            gQAndroidUserEventAgent,
            gQAndroidVmOperations,
            gQAndroidNetAgent
    );
    qemu2_android_console_setup(
            gQAndroidBatteryAgent,
            gQAndroidFingerAgent,
            gQAndroidLocationAgent,
            gQAndroidUserEventAgent,
            gQAndroidVmOperations,
            gQAndroidNetAgent);
}
