// Copyright 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android-qemu2-glue/qemu-setup.h"         // for qemu_setup_grpc
#include "android/console.h"                       // for AndroidConsoleAgents
#include "android/emulation/control/grpc_agent.h"  // for QGrpcAgent

extern const AndroidConsoleAgents* getConsoleAgents();
extern int qemu_setup_grpc(void);

int start_grpc(int port, const char* turncfg) {
    return qemu_setup_grpc();
}

static const QGrpcAgent grpcAgent = {
        .start = start_grpc,
};

extern "C" const QGrpcAgent* const gQGrpcAgent = &grpcAgent;
