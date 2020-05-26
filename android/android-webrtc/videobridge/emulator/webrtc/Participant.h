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
// #include <api/datachannelinterface.h>     // for DataChannelInterface (ptr o...
#include <api/jsep.h>                       // for IceCandidateInterface (pt...
#include <api/peer_connection_interface.h>  // for PeerConnectionInterface
#include <api/scoped_refptr.h>              // for scoped_refptr
#include <stdint.h>                         // for uint32_t, uint8_t
#include <memory>                           // for unique_ptr
#include <string>                           // for string, basic_string, ope...
#include <unordered_map>                    // for unordered_map
#include <unordered_set>                    // for unordered_set
#include <vector>                           // for vector

#include "nlohmann/json.hpp"                // for json

namespace emulator {
namespace webrtc {
class VideoCapturer;
}  // namespace webrtc
}  // namespace emulator

namespace webrtc {
class MediaStreamInterface;
class RTCError;
class RtpReceiverInterface;
class RtpSenderInterface;
class VideoTrackInterface;
}  // namespace webrtc

using json = nlohmann::json;
using rtc::scoped_refptr;
using webrtc::PeerConnectionInterface;
using webrtc::VideoTrackInterface;

namespace emulator {
namespace webrtc {

class Participant;
class Switchboard;

// A default peer connection observer that does nothing
class EmptyConnectionObserver : public ::webrtc::PeerConnectionObserver {
public:
    void OnSignalingChange(::webrtc::PeerConnectionInterface::SignalingState
                                   new_state) override {}
    void OnAddTrack(rtc::scoped_refptr<::webrtc::RtpReceiverInterface> receiver,
                    const std::vector<
                            rtc::scoped_refptr<::webrtc::MediaStreamInterface>>&
                            streams) override {}
    void OnRemoveTrack(rtc::scoped_refptr<::webrtc::RtpReceiverInterface>
                               receiver) override {}
    void OnDataChannel(rtc::scoped_refptr<::webrtc::DataChannelInterface>
                               channel) override {}
    void OnRenegotiationNeeded() override {}
    void OnIceConnectionChange(
            ::webrtc::PeerConnectionInterface::IceConnectionState new_state)
            override {}
    void OnIceGatheringChange(
            ::webrtc::PeerConnectionInterface::IceGatheringState new_state)
            override {}
    void OnIceCandidate(
            const ::webrtc::IceCandidateInterface* candidate) override {}
    void OnIceConnectionReceivingChange(bool receiving) override {}
};

// An EventForwarder forwards mouse & keyevents to the emulator.

class EventForwarder : public ::webrtc::DataChannelObserver {
public:
    EventForwarder(Participant* part,
                   scoped_refptr<::webrtc::DataChannelInterface> channel,
                   std::string label);
    ~EventForwarder();
    // The data channel state have changed.
    void OnStateChange() override;
    //  A data buffer was successfully received.
    void OnMessage(const ::webrtc::DataBuffer& buffer) override;

private:
    scoped_refptr<::webrtc::DataChannelInterface> mChannel;
    Participant* mParticipant;
    std::string mLabel;
};

// A Participant in an webrtc streaming session. This class is
// repsonsbile for driving the jsep protocol. It basically:
//
// 1. Creates the audio & video streams with shared mem handle & fps.
// 2. Do network discovery (ice etc).
// 3. Exchanging of offers between remote client.
//
// It talks with the switchboard to send/receive messages.
class Participant : public EmptyConnectionObserver,
                    public ::webrtc::CreateSessionDescriptionObserver {
public:
    Participant(
            Switchboard* board,
            std::string id,
            ::webrtc::PeerConnectionFactoryInterface* peerConnectionFactory);
    ~Participant() override;

    // PeerConnectionObserver implementation.
    void OnIceCandidate(
            const ::webrtc::IceCandidateInterface* candidate) override;

    // CreateSessionDescriptionObserver implementation.
    void OnSuccess(::webrtc::SessionDescriptionInterface* desc) override;
    // The OnFailure callback takes an RTCError, which consists of an¶
    // error code and a string.¶
    // RTCError is non-copyable, so it must be passed using std::move.¶
    // Earlier versions of the API used a string argument. This version¶
    // is deprecated; in order to let clients remove the old version, it has a¶
    // default implementation. If both versions are unimplemented, the¶
    // result will be a runtime error (stack overflow). This is intentional.¶
    void OnFailure(::webrtc::RTCError error) override;
    //void OnFailure(const std::string& error) override;
    void OnIceConnectionChange(
            ::webrtc::PeerConnectionInterface::IceConnectionState new_state)
            override;
    void IncomingMessage(json msg);
    bool AddVideoTrack(std::string handle);
    bool RemoveVideoTrack(std::string handle);
    bool AddAudioTrack(std::string grpcAddress);
    bool RemoveAudioTrack(std::string grpcAddress);
    bool Initialize();
    inline const std::string GetPeerId() const { return mPeerId; };
    void SendToBridge(json msg);
    void CreateOffer();
    void Close();

private:
    void SendMessage(json msg);
    void HandleOffer(const json& msg) const;
    void HandleCandidate(const json& msg) const;
    bool CreatePeerConnection(bool dtls);
    void AddDataChannel(const std::string& channel);
    VideoCapturer* OpenVideoCaptureDevice(const std::string& memoryHandle);

    scoped_refptr<PeerConnectionInterface> mPeerConnection;
    ::webrtc::PeerConnectionFactoryInterface* mPeerConnectionFactory;
    std::unordered_map<std::string,
                       scoped_refptr<::webrtc::MediaStreamInterface>>
            mStreams;

    std::unordered_map<std::string, std::unique_ptr<EventForwarder>>
            mEventForwarders;

    std::unordered_map<std::string, scoped_refptr<::webrtc::RtpSenderInterface>>
            mActiveVideoTracks;
    std::unordered_map<std::string, scoped_refptr<::webrtc::RtpSenderInterface>>
            mActiveAudioTracks;


    Switchboard* mSwitchboard;
    std::string mPeerId;
    uint32_t mId{0};

    const uint8_t kFps = 60;
    const std::string kStunUri = "stun:stun.l.google.com:19302";
    const std::string kAudioLabel = "emulator_audio_stream";
    const std::string kVideoLabel = "emulator_video_stream";
    const std::string kStreamLabel = "emulator_view";

    const std::unordered_set<std::string> mValidLabels{"mouse", "keyboard",
                                                       "touch"};
};
}  // namespace webrtc
}  // namespace emulator
