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

#pragma once
#include "android/console_exports.h"

/* TO BE DEPRECATED  DO NOT USE */
/* are we using the emulator in the android mode or plain qemu? */
extern int android_qemu_mode;

/* are we using android-emu libraries for a minimal configuration? */
extern int min_config_qemu_mode;
/* TO BE DEPRECATED  DO NOT USE */

#include "android/emulation/control/battery_agent.h"
#include "android/emulation/control/car_data_agent.h"
#include "android/emulation/control/clipboard_agent.h"
#include "host-common/display_agent.h"
#include "android/emulation/control/finger_agent.h"
#include "android/emulation/control/globals_agent.h"
#include "android/emulation/control/grpc_agent.h"
#include "android/emulation/control/http_proxy_agent.h"
#include "android/emulation/control/hw_control_agent.h"
#include "android/emulation/control/libui_agent.h"
#include "android/emulation/control/location_agent.h"
#include "host-common/multi_display_agent.h"
#include "android/emulation/control/net_agent.h"
#include "host-common/record_screen_agent.h"
#include "android/emulation/control/rootcanal_hci_agent.h"
#include "android/emulation/control/sensors_agent.h"
#include "android/emulation/control/telephony_agent.h"
#include "android/emulation/control/user_event_agent.h"
#include "android/emulation/control/virtual_scene_agent.h"
#include "android/emulation/control/surface_agent.h"
#include "host-common/vm_operations.h"
#include "host-common/window_agent.h"
#include "android/utils/compiler.h"
#include "emulation/control/cellular_agent.h"
ANDROID_BEGIN_HEADER

typedef struct QAndroidAutomationAgent QAndroidAutomationAgent;

// A macro used to list all the agents used by the Android Console.
// The macro takes a parameter |X| which must be a macro that takes two
// parameter as follows:  X(type, name), where |type| is the agent type
// name, and |name| is a field name. See usage below to declare
// AndroidConsoleAgents.
#define ANDROID_CONSOLE_AGENTS_LIST(X)          \
    X(QAndroidAutomationAgent, automation)      \
    X(QAndroidBatteryAgent, battery)            \
    X(QAndroidClipboardAgent, clipboard)        \
    X(QAndroidCellularAgent, cellular)          \
    X(QAndroidDisplayAgent, display)            \
    X(QAndroidEmulatorWindowAgent, emu)         \
    X(QAndroidFingerAgent, finger)              \
    X(QAndroidHttpProxyAgent, proxy)            \
    X(AndroidProxyCB, proxy_cb)                 \
    X(QAndroidLibuiAgent, libui)                \
    X(QAndroidLocationAgent, location)          \
    X(QAndroidMultiDisplayAgent, multi_display) \
    X(QAndroidNetAgent, net)                    \
    X(QAndroidRecordScreenAgent, record)        \
    X(QAndroidSensorsAgent, sensors)            \
    X(QAndroidTelephonyAgent, telephony)        \
    X(QAndroidUserEventAgent, user_event)       \
    X(QAndroidVirtualSceneAgent, virtual_scene) \
    X(QAndroidVmOperations, vm)                 \
    X(QCarDataAgent, car)                       \
    X(QGrpcAgent, grpc)                         \
    X(QAndroidHwControlAgent, hw_control)       \
    X(QAndroidGlobalVarsAgent, settings)        \
    X(QAndroidSurfaceAgent, surface)

// A structure used to group pointers to all agent interfaces used by the
// Android console.
#define ANDROID_CONSOLE_DEFINE_POINTER(type, name)  const type* name;
typedef struct AndroidConsoleAgents {
    ANDROID_CONSOLE_AGENTS_LIST(ANDROID_CONSOLE_DEFINE_POINTER)
} AndroidConsoleAgents;

// Accessor for the android console agents. The console agents are used to
// interact with the actual emulator.
CONSOLE_API const AndroidConsoleAgents* getConsoleAgents();

// Returns true if the agents are available.
CONSOLE_API bool agentsAvailable();
ANDROID_END_HEADER
