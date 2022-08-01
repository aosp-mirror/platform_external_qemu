// Copyright (C) 2019 The Android Open Source Project
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

#include <api/data_channel_interface.h>     // for DataChannelInterface (ptr...
#include <api/jsep.h>                       // for IceCandidateInterface (pt...
#include <api/peer_connection_interface.h>  // for PeerConnectionInterface
#include <api/scoped_refptr.h>              // for scoped_refptr
#include <stdint.h>                         // for uint32_t
#include <condition_variable>               // for condition_variable
#include <memory>                           // for unique_ptr, shared_ptr
#include <string>                           // for string
#include <unordered_map>                    // for unordered_map

#include "emulator/webrtc/export.h"
#include "android/console.h"
#include "android/emulation/control/keyboard/EmulatorKeyEventSender.h"
#include "android/emulation/control/keyboard/TouchEventSender.h"
#include "emulator/webrtc/RtcConnection.h"  // for json, RtcConnection (ptr ...
#include "emulator_controller.grpc.pb.h"    // for EmulatorController
#include "nlohmann/json.hpp"                // for json

namespace emulator {
namespace webrtc {
class VideoCapturer;
class VideoTrackReceiver;
}  // namespace webrtc
}  // namespace emulator
namespace webrtc {
class MediaStreamInterface;
class RTCError;
class RtpSenderInterface;
class VideoTrackInterface;
}  // namespace webrtc

using json = nlohmann::json;
using rtc::scoped_refptr;
using webrtc::PeerConnectionInterface;
using webrtc::VideoTrackInterface;

namespace emulator {
namespace webrtc {

// Used data channel labels, use lowercase labels otherwise the JS engine will
// break.
enum class DataChannelLabel { mouse, keyboard, touch, adb };

// A default peer connection observer that does nothing
class EmptyConnectionObserver : public ::webrtc::PeerConnectionObserver {
public:
    ANDROID_WEBRTC_EXPORT void OnSignalingChange(::webrtc::PeerConnectionInterface::SignalingState
                                   new_state) override {}
    ANDROID_WEBRTC_EXPORT void OnRenegotiationNeeded() override {}
    ANDROID_WEBRTC_EXPORT void OnIceGatheringChange(
            ::webrtc::PeerConnectionInterface::IceGatheringState new_state)
            override {}
    ANDROID_WEBRTC_EXPORT void OnIceCandidate(
            const ::webrtc::IceCandidateInterface* candidate) override {}
};

// An EventForwarder forwards mouse & keyevents to the emulator.

class EventForwarder : public ::webrtc::DataChannelObserver {
public:
    ANDROID_WEBRTC_EXPORT EventForwarder(const AndroidConsoleAgents* agents,
                   scoped_refptr<::webrtc::DataChannelInterface> channel,
                   DataChannelLabel label);
    ANDROID_WEBRTC_EXPORT ~EventForwarder();
    // The data channel state have changed.
    ANDROID_WEBRTC_EXPORT void OnStateChange() override;
    //  A data buffer was successfully received.
    ANDROID_WEBRTC_EXPORT void OnMessage(const ::webrtc::DataBuffer& buffer) override;

private:
    const AndroidConsoleAgents* mAgents;
    android::emulation::control::keyboard::EmulatorKeyEventSender
            mKeyEventSender;
    android::emulation::control::TouchEventSender mTouchEventSender;
    scoped_refptr<::webrtc::DataChannelInterface> mChannel;
    DataChannelLabel mLabel;
};

// A Participant in an webrtc streaming session. This class is
// repsonsbile for driving the jsep protocol. It basically:
//
// 1. Creates the audio & video streams with shared mem handle & fps.
// 2. Do network discovery (ice etc).
// 3. Exchanging of offers between remote client.
//
// It talks with the rtc connection to send/receive messages.
class Participant : public EmptyConnectionObserver {
public:
    ANDROID_WEBRTC_EXPORT Participant(RtcConnection* board,
                std::string id,
                json rtcConfig,
                VideoTrackReceiver* videoReceiver);
    ANDROID_WEBRTC_EXPORT ~Participant() override;

    // PeerConnectionObserver implementation.
    ANDROID_WEBRTC_EXPORT void OnIceCandidate(
            const ::webrtc::IceCandidateInterface* candidate) override;
    ANDROID_WEBRTC_EXPORT void OnConnectionChange(
            ::webrtc::PeerConnectionInterface::PeerConnectionState new_state)
            override;

    ANDROID_WEBRTC_EXPORT void OnIceConnectionChange(
            ::webrtc::PeerConnectionInterface::IceConnectionState new_state)
            override;

    ANDROID_WEBRTC_EXPORT void OnAddStream(
            rtc::scoped_refptr<::webrtc::MediaStreamInterface> stream) override;
    ANDROID_WEBRTC_EXPORT void OnDataChannel(rtc::scoped_refptr<::webrtc::DataChannelInterface>
                               channel) override;
    ANDROID_WEBRTC_EXPORT void ReceivedSessionDescription(
            ::webrtc::SessionDescriptionInterface* desc);
    ANDROID_WEBRTC_EXPORT void IncomingMessage(json msg);
    ANDROID_WEBRTC_EXPORT bool AddVideoTrack(int displayId);
    ANDROID_WEBRTC_EXPORT bool AddAudioTrack(const std::string& audioDumpFile = "");

    ANDROID_WEBRTC_EXPORT bool Initialize();
    ANDROID_WEBRTC_EXPORT void SendToDataChannel(DataChannelLabel label, std::string data);
    ANDROID_WEBRTC_EXPORT void CreateOffer();
    ANDROID_WEBRTC_EXPORT void Close();
    ANDROID_WEBRTC_EXPORT void WaitForClose();

    ANDROID_WEBRTC_EXPORT rtc::scoped_refptr<::webrtc::DataChannelInterface> GetDataChannel(
            DataChannelLabel label);
    ANDROID_WEBRTC_EXPORT inline const std::string GetPeerId() const { return mPeerId; };

private:
    void DoClose();
    void SendMessage(json msg);
    void HandleOffer(const json& msg);
    void HandleCandidate(const json& msg);
    bool CreatePeerConnection(const json& description);
    void AddDataChannel(const DataChannelLabel channel);
    VideoCapturer* OpenVideoCaptureDevice(const std::string& memoryHandle);

    scoped_refptr<PeerConnectionInterface> mPeerConnection;
    std::unordered_map<DataChannelLabel, std::unique_ptr<EventForwarder>>
            mEventForwarders;

    std::unordered_map<std::string,
                       scoped_refptr<::webrtc::DataChannelInterface>>
            mDataChannels;
    std::unordered_map<std::string, scoped_refptr<::webrtc::RtpSenderInterface>>
            mActiveTracks;

    InprocessRefAudioSource mAudioSource;
    std::vector<InprocessRefVideoSource> mVideoSources;

    RtcConnection* mRtcConnection;
    VideoTrackReceiver* mVideoReceiver;
    json mRtcConfig;
    std::string mPeerId;
    std::atomic<bool> mClosed{false};

    std::mutex mClosedMutex;
    std::condition_variable mCvClosed;

public:
    const static std::string kAudioTrack;
    const static std::string kVideoTrack;
};
}  // namespace webrtc
}  // namespace emulator
