// Copyright (C) 2021 The Android Open Source Project
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

#include <api/peer_connection_interface.h>  // for PeerConnectionInterface
#include <string>                           // for string

#include "nlohmann/json.hpp"  // for json
#include "android/emulation/control/webrtc-exports.h"

namespace emulator {
namespace webrtc {

using nlohmann::json;
using ::webrtc::PeerConnectionInterface;

class WEBRTC_API RtcConfig {
public:
    static PeerConnectionInterface::RTCConfiguration parse(
            json rtcConfiguration);
    static PeerConnectionInterface::RTCConfiguration parse(
            std::string rtcConfiguration);
};
}  // namespace webrtc
}  // namespace emulator
