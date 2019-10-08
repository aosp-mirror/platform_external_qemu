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

#include "android-qemu2-glue/qemu-control-impl.h"

#include "android/emulation/control/grpc_agent.h"
#include <stdio.h>

#ifdef ANDROID_GRPC
#include "android/emulation/control/GrpcServices.h"

extern const AndroidConsoleAgents* getConsoleAgents();

int start_grpc(int port, const char* turncfg) {
     return android::emulation::control::GrpcServices::setup(port, getConsoleAgents(), turncfg);
}
#else
int start_grpc(int port, const char* turncfg) {
    printf("GRPC endpoint not available\n");
    return -1;
}
#endif


static const QGrpcAgent grpcAgent = {
        .start = start_grpc,
};

const QGrpcAgent* const gQGrpcAgent = &grpcAgent;