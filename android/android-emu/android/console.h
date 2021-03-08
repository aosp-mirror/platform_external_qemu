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

#include "android/emulation/control/battery_agent.h"
#include "android/emulation/control/car_data_agent.h"
#include "android/emulation/control/clipboard_agent.h"
#include "android/emulation/control/display_agent.h"
#include "android/emulation/control/finger_agent.h"
#include "android/emulation/control/grpc_agent.h"
#include "android/emulation/control/http_proxy_agent.h"
#include "android/emulation/control/libui_agent.h"
#include "android/emulation/control/location_agent.h"
#include "android/emulation/control/multi_display_agent.h"
#include "android/emulation/control/net_agent.h"
#include "android/emulation/control/record_screen_agent.h"
#include "android/emulation/control/sensors_agent.h"
#include "android/emulation/control/telephony_agent.h"
#include "android/emulation/control/user_event_agent.h"
#include "android/emulation/control/virtual_scene_agent.h"
#include "android/emulation/control/vm_operations.h"
#include "android/emulation/control/window_agent.h"
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
    X(QGrpcAgent, grpc)

// A structure used to group pointers to all agent interfaces used by the
// Android console.
#define ANDROID_CONSOLE_DEFINE_POINTER(type, name) const type* name;
typedef struct AndroidConsoleAgents {
    ANDROID_CONSOLE_AGENTS_LIST(ANDROID_CONSOLE_DEFINE_POINTER)
} AndroidConsoleAgents;

// Generic entry point to start an android console.
// QEMU implementations should populate |*agents| with QEMU specific
// functions. Takes ownership of |agents|.
extern int android_console_start(int port, const AndroidConsoleAgents* agents);


// Accessor for the android console agents. The console agents are used to interact with the actual emulator.
const AndroidConsoleAgents* getConsoleAgents();
ANDROID_END_HEADER
