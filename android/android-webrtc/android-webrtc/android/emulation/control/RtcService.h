// Copyright (C) 2018 The Android Open Source Project
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
#include "android/console.h"  // for AndroidConsoleAgents
#include "android/emulation/control/webrtc-exports.h"

namespace grpc {
class Service;
}  // namespace grpc

namespace android {
namespace emulation {
namespace control {
class RtcBridge;

WEBRTC_API grpc::Service* getRtcService(const AndroidConsoleAgents* agents,
                             int adbPort,
                             const char* turncfg);
WEBRTC_API grpc::Service* getRtcService(RtcBridge* bridge);

WEBRTC_API grpc::Service* getRtcService(const char* turncfg,
                                const char* audioDump,
                                bool verbose);
}  // namespace control
}  // namespace emulation
}  // namespace android
