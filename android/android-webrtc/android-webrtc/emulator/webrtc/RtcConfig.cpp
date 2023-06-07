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
#include "emulator/webrtc/RtcConfig.h"

#include "aemu/base/Log.h"

#include <stdint.h>          // for uint8_t
#include <initializer_list>  // for initializer_list
#include <vector>            // for vector

namespace emulator {
namespace webrtc {

using nlohmann::json;
using ::webrtc::PeerConnectionInterface;

const std::string kDefaultStunUri = "stun:stun.l.google.com:19302";

static PeerConnectionInterface::IceServer parseIce(json desc) {
    PeerConnectionInterface::IceServer ice;
    if (desc.count("credential")) {
        ice.password = desc["credential"];
    }
    if (desc.count("username")) {
        ice.username = desc["username"];
    }
    if (desc.count("urls")) {
        auto urls = desc["urls"];
        if (urls.is_string()) {
            ice.urls.push_back(urls.get<std::string>());
        } else {
            for (const auto& uri : urls) {
                if (uri.is_string()) {
                    ice.urls.push_back(uri.get<std::string>());
                }
            }
        }
    }
    return ice;
}

PeerConnectionInterface::RTCConfiguration RtcConfig::parse(
        json rtcConfiguration) {
    PeerConnectionInterface::RTCConfiguration configuration;

    json iceServers;
    // This is what you will get from the google turn sever
    if (rtcConfiguration.count("iceServers")) {
        iceServers = rtcConfiguration["iceServers"];
    }

    // This is what you will get from twilio
    // See: https://www.twilio.com/docs/stun-turn
    if (rtcConfiguration.count("ice_servers")) {
        iceServers = rtcConfiguration["ice_servers"];
    }

    if (!iceServers.empty()) {
        for (const auto& iceServer : iceServers) {
            configuration.servers.push_back(parseIce(iceServer));
        }
    } else {
        LOG(WARNING) << "RtcConfig iceServers is empty.";
    }

    // Modern webrtc with multiple audio & video channels.
    configuration.sdp_semantics = ::webrtc::SdpSemantics::kUnifiedPlan;

    // ice candidate pool size
    const std::string kIceCandidatePoolSize = "iceCandidatePoolSize";
    if (rtcConfiguration.count(kIceCandidatePoolSize) &&
        rtcConfiguration[kIceCandidatePoolSize].is_number_integer()) {
        configuration.ice_candidate_pool_size =
                rtcConfiguration[kIceCandidatePoolSize];
    }

    // bundle policy
    const std::string kBundlePolicy = "bundlePolicy";
    if (rtcConfiguration.count(kBundlePolicy)) {
        std::string policy = rtcConfiguration[kBundlePolicy];
        if (policy == "balanced") {
            configuration.bundle_policy = PeerConnectionInterface::
                    BundlePolicy::kBundlePolicyBalanced;
        } else if (policy == "max-compat") {
            configuration.bundle_policy = PeerConnectionInterface::
                    BundlePolicy::kBundlePolicyMaxCompat;
        } else if (policy == "max-bundle") {
            configuration.bundle_policy = PeerConnectionInterface::
                    BundlePolicy::kBundlePolicyMaxBundle;
        }
    }

    const std::string kIceTransportPolicy = "iceTransportPolicy";
    if (rtcConfiguration.count(kIceTransportPolicy)) {
        std::string policy = rtcConfiguration[kIceTransportPolicy];
        if (policy == "relay") {
            configuration.type =
                    PeerConnectionInterface::IceTransportsType::kRelay;
        } else if (policy == "all") {
            configuration.type =
                    PeerConnectionInterface::IceTransportsType::kAll;
        }
    }

    const std::string kRtcpMuxPolicy = "rtcpMuxPolicy";
    if (rtcConfiguration.count(kRtcpMuxPolicy)) {
        std::string policy = rtcConfiguration[kRtcpMuxPolicy];
        if (policy == "require") {
            configuration.rtcp_mux_policy = PeerConnectionInterface::
                    RtcpMuxPolicy::kRtcpMuxPolicyRequire;
        } else if (policy == "negotiate") {
            configuration.rtcp_mux_policy = PeerConnectionInterface::
                    RtcpMuxPolicy::kRtcpMuxPolicyNegotiate;
        }
    }
    // Let's add at least a default stun server if none is present.
    if (configuration.servers.empty()) {
        PeerConnectionInterface::IceServer server;
        server.uri = kDefaultStunUri;
        configuration.servers.push_back(server);
    }
    return configuration;
}

PeerConnectionInterface::RTCConfiguration RtcConfig::parse(
        std::string rtcConfiguration) {
    json config = json::parse(rtcConfiguration, nullptr, false);
    if (!config.is_discarded())
        return parse(config);
    return {};
}
}  // namespace webrtc
}  // namespace emulator
