// Copyright (C) 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/snapshot/Postmortem.h"

#include "android/android.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/base/sockets/SocketUtils.h"

#include <stdio.h>

namespace android {
namespace postmortem {
bool track(int pid, const char *snapshot_name) {
    printf("adb port %d\n", android_adb_port);
    int s = base::socketTcp4LoopbackClient(android_adb_port);
    if (s < 0) {
        s = base::socketTcp6LoopbackClient(android_adb_port);
    }
    if (s < 0) {
        return false;
    }
    base::ScopedSocket ss(s);
    base::socketSetBlocking(s);
    char jdwp_connect[30];
    sprintf(jdwp_connect, "jdwp:%d", pid);
#define _SEND_BUFFER(buffer, len) { if (!base::socketSendAll(s, buffer, len)) return false; }
#define _SEND_STR(buffer) _SEND_BUFFER(buffer, strlen(buffer))
    _SEND_STR(jdwp_connect);
    const char* jdwp_handshake = "JDWP-Handshake";
    _SEND_STR(jdwp_handshake);
    char recvbuffer[100];
#define _RECV_BUFFER(buffer) { if (!base::socketRecvAll(s, buffer, sizeof(buffer))) return false; }
    _RECV_BUFFER(recvbuffer);
    if (strcmp(jdwp_handshake, recvbuffer) != 0) {
        return false;
    }
    return true;
}
}
}