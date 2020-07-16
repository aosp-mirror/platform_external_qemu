// Copyright (C) 2020 The Android Open Source Project
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
#include "android-qemu2-glue/qemu-console-factory.h"

#include "android/console.h"                                // for ANDROID_C...
#include "android/emulation/control/AndroidAgentFactory.h"  // for initializ...
#include "android/emulation/control/battery_agent.h"        // for QAndroidB...
#include "android/emulation/control/car_data_agent.h"       // for QCarDataA...
#include "android/emulation/control/cellular_agent.h"       // for QAndroidC...
#include "android/emulation/control/clipboard_agent.h"      // for QAndroidC...
#include "android/emulation/control/display_agent.h"        // for QAndroidD...
#include "android/emulation/control/finger_agent.h"         // for QAndroidF...
#include "android/emulation/control/grpc_agent.h"           // for QGrpcAgent
#include "android/emulation/control/http_proxy_agent.h"     // for QAndroidH...
#include "android/emulation/control/libui_agent.h"          // for QAndroidL...
#include "android/emulation/control/location_agent.h"       // for QAndroidL...
#include "android/emulation/control/multi_display_agent.h"  // for QAndroidM...
#include "android/emulation/control/net_agent.h"            // for QAndroidN...
#include "android/emulation/control/record_screen_agent.h"  // for QAndroidR...
#include "android/emulation/control/sensors_agent.h"        // for QAndroidS...
#include "android/emulation/control/telephony_agent.h"      // for QAndroidT...
#include "android/emulation/control/user_event_agent.h"     // for QAndroidU...
#include "android/emulation/control/virtual_scene_agent.h"  // for QAndroidV...
#include "android/emulation/control/vm_operations.h"        // for QAndroidV...
#include "android/emulation/control/window_agent.h"         // for QAndroidE...

extern const QAndroidAutomationAgent* const gQAndroidAutomationAgent;

// Defined in .../emulation/control/battery_agent.h
extern const QAndroidBatteryAgent* const gQAndroidBatteryAgent;

// Defined in android/qemu-cellular-agent.c
extern const QAndroidCellularAgent* const gQAndroidCellularAgent;

// Defined in qemu-clipboard-agent-impl.cpp
extern const QAndroidClipboardAgent* const gQAndroidClipboardAgent;

// Defined in android/qemu-finger-agent.c
extern const QAndroidFingerAgent* const gQAndroidFingerAgent;

// Defined in android/qemu-location-agent-impl.c
extern const QAndroidLocationAgent* const gQAndroidLocationAgent;

// Defined in android/qemu-http-proxy-agent-impl.c
extern const QAndroidHttpProxyAgent* const gQAndroidHttpProxyAgent;

// Defined in android/qemu-record-screen-agent.c
extern const QAndroidRecordScreenAgent* const gQAndroidRecordScreenAgent;

// Defined in android/qemu-sensors-agent.cpp
extern const QAndroidSensorsAgent* const gQAndroidSensorsAgent;

// Defined in android/qemu-telephony-agent.c
extern const QAndroidTelephonyAgent* const gQAndroidTelephonyAgent;

// Defined in android-qemu2-glue/qemu-user-event-agent-impl.c
extern const QAndroidUserEventAgent* const gQAndroidUserEventAgent;

// Defined in android/qemu-virtual-scene-agent.cpp
extern const QAndroidVirtualSceneAgent* const gQAndroidVirtualSceneAgent;

// Defined in android-qemu2-glue/qemu-net-agent-impl.c
extern const QAndroidNetAgent* const gQAndroidNetAgent;

// Defined in android-qemu2-glue/qemu-display-agent-impl.cpp
extern const QAndroidDisplayAgent* const gQAndroidDisplayAgent;
extern const QCarDataAgent* const gQCarDataAgent;
extern const QGrpcAgent* const gQGrpcAgent;
extern const QAndroidMultiDisplayAgent* const gQAndroidMultiDisplayAgent;

extern const QAndroidEmulatorWindowAgent* const gQAndroidEmulatorWindowAgent;

// Do not use, get a reference to the battery agent through the getConsoleAgents
// function.
extern const QAndroidVmOperations* const gQAndroidVmOperations;

// Do not use, get a reference to the battery agent through the getConsoleAgents
// function.
extern const QAndroidMultiDisplayAgent* const gQAndroidMultiDisplayAgent;
extern const QAndroidLibuiAgent* const gQAndroidLibuiAgent;

#define ANDROID_DEFINE_CONSOLE_GETTER_IMPL(typ, name) \
    const typ* const android_get_##typ() const { return g##typ; };

class QemuAndroidConsoleAgentFactory
    : public android::emulation::AndroidConsoleFactory {
    ANDROID_CONSOLE_AGENTS_LIST(ANDROID_DEFINE_CONSOLE_GETTER_IMPL)
};

void injectConsoleAgents(const char* factory) {
    android::emulation::injectConsoleAgents(QemuAndroidConsoleAgentFactory());
}
