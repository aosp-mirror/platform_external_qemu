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
#include "android/utils/looper.h"

#include "android/emulation/control/battery_agent.h"
#include "android/emulation/control/car_data_agent.h"
#include "android/emulation/control/finger_agent.h"
#include "android/emulation/control/libui_agent.h"
#include "android/emulation/control/location_agent.h"
#include "android/emulation/control/http_proxy_agent.h"
#include "android/emulation/control/net_agent.h"
#include "android/emulation/control/telephony_agent.h"
#include "android/emulation/control/user_event_agent.h"
#include "android/emulation/control/vm_operations.h"
#include "android/emulation/control/window_agent.h"
#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// A macro used to list all the agents used by the Android Console.
// The macro takes a parameter |X| which must be a macro that takes two
// parameter as follows:  X(type, name), where |type| is the agent type
// name, and |name| is a field name. See usage below to declare
// AndroidConsoleAgents.
#define ANDROID_CONSOLE_AGENTS_LIST(X)    \
    X(QAndroidBatteryAgent, battery)      \
    X(QAndroidEmulatorWindowAgent, emu)   \
    X(QAndroidFingerAgent, finger)        \
    X(QAndroidLocationAgent, location)    \
    X(QAndroidHttpProxyAgent, proxy)      \
    X(QAndroidTelephonyAgent, telephony)  \
    X(QAndroidUserEventAgent, user_event) \
    X(QAndroidVmOperations, vm)           \
    X(QAndroidNetAgent, net)              \
    X(QAndroidLibuiAgent, libui)          \
    X(QCarDataAgent, car)

// A structure used to group pointers to all agent interfaces used by the
// Android console.
#define ANDROID_CONSOLE_DEFINE_POINTER(type,name) const type* name;
typedef struct AndroidConsoleAgents {
    ANDROID_CONSOLE_AGENTS_LIST(ANDROID_CONSOLE_DEFINE_POINTER)
} AndroidConsoleAgents;

// Generic entry point to start an android console.
// QEMU implementations should populate |*agents| with QEMU specific
// functions. Takes ownership of |agents|.
extern int android_console_start(int port, const AndroidConsoleAgents* agents);

typedef struct ControlGlobalRec_*  ControlGlobal;

typedef struct ControlClientRec_*  ControlClient;

typedef struct {
    int           host_port;
    int           host_udp;
    unsigned int  guest_ip;
    int           guest_port;
} RedirRec, *Redir;


#define MAX_CONSOLE_MSG_LENGTH 4096

typedef int Socket;
typedef const struct CommandDefRec_* CommandDef;

typedef struct ControlClientRec_
{
    struct ControlClientRec_*  next;       /* next client in list           */
    Socket                     sock;       /* socket used for communication */
    // The loopIo currently communicating over |sock|. May change over the
    // lifetime of a ControlClient.
    LoopIo* loopIo;
    ControlGlobal              global;
    char                       finished;
    char                       buff[ MAX_CONSOLE_MSG_LENGTH ];
    int                        buff_len;
    CommandDef                 commands;

    // Used for those who provide their own mechanism to write back data.
    void*                      opaque;
    void (*write)(ControlClient client, const char* line, int len);
} ControlClientRec;

ControlClient control_client_create_empty();

typedef struct ControlGlobalRec_
{
    // Interfaces to call into QEMU specific code.
#define ANDROID_CONSOLE_DEFINE_FIELD(type, name) type name ## _agent [1];
    ANDROID_CONSOLE_AGENTS_LIST(ANDROID_CONSOLE_DEFINE_FIELD)

    /* IO */
    Looper* looper;
    LoopIo* listen4_loopio;
    LoopIo* listen6_loopio;

    /* the list of current clients */
    ControlClient   clients;

    /* the list of redirections currently active */
    Redir     redirs;
    int       num_redirs;
    int       max_redirs;

} ControlGlobalRec;

void control_client_do_command(ControlClient client);


ANDROID_END_HEADER
