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

#include <stdio.h>
#include "android/cmdline-option.h"
#include "android/emulation/control/grpc_agent.h"

#ifdef ANDROID_WEBRTC
#include "android/emulation/control/WebRtcBridge.h"  // for WebRtcBridge
#endif

#include "android/emulation/control/GrpcServices.h"

extern const AndroidConsoleAgents* getConsoleAgents();

int start_grpc(int port, const char* turncfg) {
    android::emulation::control::RtcBridge* rtc_bridge = nullptr;
#ifdef ANDROID_WEBRTC
    std::string turn = "";
    if (android_cmdLineOptions->turncfg) {
        turn = android_cmdLineOptions->turncfg;
    }
    rtc_bridge = android::emulation::control::WebRtcBridge::create(
            0, getConsoleAgents(), turn);
#else
    rtc_bridge = new android::emulation::control::NopRtcBridge();
#endif
    auto service = android::emulation::control::GrpcServices::setup(
            port, getConsoleAgents(), rtc_bridge, "forward");
    return service ? service->port() : -1;
}

static const QGrpcAgent grpcAgent = {
        .start = start_grpc,
};

const QGrpcAgent* const gQGrpcAgent = &grpcAgent;