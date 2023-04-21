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

#include "aemu/base/Log.h"                               // for LogStream...
#include "android/console.h"                                // for ANDROID_C...
#include "android/emulation/control/AgentLogger.h"          // for AndroidLo...
#include "android/emulation/control/AndroidAgentFactory.h"  // for injectCon...
#include "android/emulation/control/battery_agent.h"        // for QAndroidB...
#include "android/emulation/control/car_data_agent.h"       // for QCarDataA...
#include "android/emulation/control/cellular_agent.h"       // for QAndroidC...
#include "android/emulation/control/clipboard_agent.h"      // for QAndroidC...
#include "android/emulation/control/finger_agent.h"         // for QAndroidF...
#include "android/emulation/control/grpc_agent.h"           // for QGrpcAgent
#include "android/emulation/control/http_proxy_agent.h"     // for QAndroidH...
#include "android/emulation/control/hw_control_agent.h"
#include "android/emulation/control/libui_agent.h"          // for QAndroidL...
#include "android/emulation/control/location_agent.h"       // for QAndroidL...
#include "android/emulation/control/net_agent.h"            // for QAndroidN...
#include "android/emulation/control/sensors_agent.h"        // for QAndroidS...
#include "android/emulation/control/telephony_agent.h"      // for QAndroidT...
#include "android/emulation/control/user_event_agent.h"     // for QAndroidU...
#include "android/emulation/control/virtual_scene_agent.h"  // for QAndroidV...
#include "host-common/display_agent.h"        // for QAndroidD...
#include "host-common/multi_display_agent.h"  // for QAndroidM...
#include "host-common/record_screen_agent.h"  // for QAndroidR...
#include "host-common/vm_operations.h"        // for QAndroidV...
#include "host-common/window_agent.h"         // for QAndroidE...
#include "android/utils/debug.h"                            // for VERBOSE_C...

extern "C" const QAndroidAutomationAgent* const gQAndroidAutomationAgent;

// Defined in .../emulation/control/battery_agent.h
extern "C" const QAndroidBatteryAgent* const gQAndroidBatteryAgent;

// Defined in android/qemu-cellular-agent.c
extern "C" const QAndroidCellularAgent* const gQAndroidCellularAgent;

// Defined in qemu-clipboard-agent-impl.cpp
extern "C" const QAndroidClipboardAgent* const gQAndroidClipboardAgent;

// Defined in android/qemu-finger-agent.c
extern "C" const QAndroidFingerAgent* const gQAndroidFingerAgent;

// Defined in android/qemu-location-agent-impl.c
extern "C" const QAndroidLocationAgent* const gQAndroidLocationAgent;

// Defined in android/qemu-http-proxy-agent-impl.c
extern "C" const QAndroidHttpProxyAgent* const gQAndroidHttpProxyAgent;
extern "C" const AndroidProxyCB* const gAndroidProxyCB;

// Defined in android/qemu-record-screen-agent.c
extern "C" const QAndroidRecordScreenAgent* const gQAndroidRecordScreenAgent;

// Defined in android/qemu-sensors-agent.cpp
extern "C" const QAndroidSensorsAgent* const gQAndroidSensorsAgent;

// Defined in android/qemu-telephony-agent.c
extern "C" const QAndroidTelephonyAgent* const gQAndroidTelephonyAgent;

// Defined in android-qemu2-glue/qemu-user-event-agent-impl.c
extern "C" const QAndroidUserEventAgent* const gQAndroidUserEventAgent;

// Defined in android/qemu-virtual-scene-agent.cpp
extern "C" const QAndroidVirtualSceneAgent* const gQAndroidVirtualSceneAgent;

// Defined in android-qemu2-glue/qemu-net-agent-impl.c
extern "C" const QAndroidNetAgent* const gQAndroidNetAgent;

// Defined in android-qemu2-glue/qemu-display-agent-impl.cpp
extern "C" const QAndroidDisplayAgent* const gQAndroidDisplayAgent;

// Defined in android-qemu2-glue/qemu-car-data-agent-impl.cpp
extern "C" const QCarDataAgent* const gQCarDataAgent;

// Defined in android-qemu2-glue/qemu-grpc-agent-impl.cpp
extern "C" const QGrpcAgent* const gQGrpcAgent;

// android-qemu2-glue/qemu-multi-display-agent-impl.cpp
extern "C" const QAndroidMultiDisplayAgent* const gQAndroidMultiDisplayAgent;

extern "C" const QAndroidVmOperations* const gQAndroidVmOperations;

extern "C" const QAndroidMultiDisplayAgent* const gQAndroidMultiDisplayAgent;

// Defined in android-qemu2-glue/qemu-hw-control-agent-impl.cpp
extern "C" const QAndroidHwControlAgent* const gQAndroidHwControlAgent;

// Defined in android-qemu2-glue/qemu-globals-agent-impl.cpp
extern "C" const QAndroidGlobalVarsAgent* const gQAndroidGlobalVarsAgent;


#define ANDROID_DEFINE_CONSOLE_GETTER_IMPL(typ) \
    const typ* const android_get_##typ() const override { return g##typ; };

const QAndroidEmulatorWindowAgent* const getEmulatorWindowAgent();

const QAndroidLibuiAgent* const getQAndroidLibuiAgent();

#define ANDROID_AGENTS_LIST(X)   \
    X(QAndroidAutomationAgent)   \
    X(QAndroidBatteryAgent)      \
    X(QAndroidClipboardAgent)    \
    X(QAndroidCellularAgent)     \
    X(QAndroidDisplayAgent)      \
    X(QAndroidFingerAgent)       \
    X(QAndroidHttpProxyAgent)    \
    X(QAndroidLocationAgent)     \
    X(QAndroidMultiDisplayAgent) \
    X(QAndroidNetAgent)          \
    X(QAndroidRecordScreenAgent) \
    X(QAndroidSensorsAgent)      \
    X(QAndroidTelephonyAgent)    \
    X(QAndroidUserEventAgent)    \
    X(QAndroidVirtualSceneAgent) \
    X(QAndroidVmOperations)      \
    X(QCarDataAgent)             \
    X(QGrpcAgent)                \
    X(QAndroidHwControlAgent)    \
    X(QAndroidGlobalVarsAgent)

class QemuAndroidConsoleAgentFactory
    : public android::emulation::AndroidConsoleFactory {
    ANDROID_AGENTS_LIST(ANDROID_DEFINE_CONSOLE_GETTER_IMPL)

    // These are exported from shared memory modules, and hence cannot be
    // directly refering to constants..
    const QAndroidEmulatorWindowAgent* const
    android_get_QAndroidEmulatorWindowAgent() const override {
        return getEmulatorWindowAgent();
    }

    const QAndroidLibuiAgent* const android_get_QAndroidLibuiAgent()
            const override {
        return getQAndroidLibuiAgent();
    }
};

using android::emulation::AndroidLoggingConsoleFactory;

void injectQemuConsoleAgents(const char* factory) {
    injectConsoleAgents(QemuAndroidConsoleAgentFactory());
    if (VERBOSE_CHECK(events)) {
        LOG(INFO) << "-- Injecting logging agents for user events.";
        injectConsoleAgents(AndroidLoggingConsoleFactory());
    }
}
