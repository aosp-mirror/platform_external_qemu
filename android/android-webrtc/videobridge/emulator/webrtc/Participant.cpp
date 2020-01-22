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
#include "Participant.h"

#include <absl/types/optional.h>                        // for optional
#include <api/mediastreaminterface.h>                   // for MediaStreamTr...
#include <api/rtcerror.h>                               // for RTCError, RTC...
#include <api/rtpsenderinterface.h>                     // for RtpSenderInte...
#include <emulator/webrtc/capture/VideoShareFactory.h>  // for VideoShareFac...
#include <media/base/device.h>                          // for Device
#include <media/base/videocapturer.h>                   // for VideoCapturer
#include <media/engine/webrtcvideocapturer.h>           // for WebRtcVideoCa...
#include <p2p/base/portallocator.h>                     // for PortAllocator
#include <rtc_base/checks.h>                            // for FatalLogCall
#include <rtc_base/copyonwritebuffer.h>                 // for CopyOnWriteBu...
#include <rtc_base/logging.h>                           // for RTC_LOG
#include <rtc_base/refcountedobject.h>                  // for RefCountedObject
#include <rtc_base/rtccertificategenerator.h>           // for RTCCertificat...
#include <utility>                                      // for move, pair

#include "emulator/webrtc/Switchboard.h"                // for Switchboard
#include "nlohmann/json.hpp"                            // for basic_json<>:...
#include "rtc_base/third_party/base64/base64.h"         // for Base64

using json = nlohmann::json;
using MediaStreamPair =
        std::pair<std::string, scoped_refptr<webrtc::MediaStreamInterface>>;

#define DTLS_ON true
#define DTLS_OFF false
namespace emulator {
namespace webrtc {

class DummySetSessionDescriptionObserver
    : public ::webrtc::SetSessionDescriptionObserver {
public:
    static DummySetSessionDescriptionObserver* Create() {
        return new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
    }
    virtual void OnSuccess() {}
    virtual void OnFailure(const std::string& error) {}

protected:
    DummySetSessionDescriptionObserver() {}
    ~DummySetSessionDescriptionObserver() {}
};

EventForwarder::EventForwarder(
        Participant* part,
        scoped_refptr<::webrtc::DataChannelInterface> channel,
        std::string label)
    : mParticipant(part), mChannel(channel), mLabel(label) {
    channel->RegisterObserver(this);
}

EventForwarder::~EventForwarder() {
    mChannel->UnregisterObserver();
}

void EventForwarder::OnStateChange() {}

void EventForwarder::OnMessage(const ::webrtc::DataBuffer& buffer) {
    std::string encoded;
    rtc::Base64::EncodeFromArray(buffer.data.cdata(), buffer.data.size(),
                                 &encoded);
    json msg;
    msg["msg"] = encoded;
    msg["label"] = mLabel;

    mParticipant->SendToBridge(msg);
}

Participant::~Participant() {
    if (mPeerConnection) {
        mPeerConnection->Close();
    }
}

static int sId = 0;

Participant::Participant(
        Switchboard* board,
        std::string id,
        ::webrtc::PeerConnectionFactoryInterface* peerConnectionFactory)
    : mSwitchboard(board),
      mPeerId(id),
      mPeerConnectionFactory(peerConnectionFactory) {
    mId = sId++;
}

void Participant::OnIceConnectionChange(
        ::webrtc::PeerConnectionInterface::IceConnectionState new_state) {
    RTC_LOG(INFO) << "Participant: " << mId
                  << ", OnIceConnectionChange: " << new_state;
    switch (new_state) {
        case ::webrtc::PeerConnectionInterface::kIceConnectionFailed:
        case ::webrtc::PeerConnectionInterface::kIceConnectionDisconnected:
            mEventForwarders.clear();
            mStreams.clear();
            RTC_LOG(INFO) << "Disconnected! Time to clean up, peer: " << mId;
            // We cannot call close from the signaling thread, lest we lock up
            // the world.
            // TODO(jansene): Figure out if this can directly happen on the
            // worker thread.
            mSwitchboard->rtcConnectionDropped(mPeerId);
            break;
        case ::webrtc::PeerConnectionInterface::IceConnectionState::
                kIceConnectionClosed:
            // Boo! we are closed, peer connection will be garbage collected on
            // other thread, as we cannot make ourself invalid on the calling
            // thread that is actually using us.
            RTC_LOG(INFO) << "Closed! Time to erase peer: " << mId;
            mSwitchboard->rtcConnectionClosed(mPeerId);
            break;
        default:;  // Nothing ..
    }
}

void Participant::Close() {
    if (mPeerConnection) {
        mPeerConnection->Close();
    }
}

void Participant::OnIceCandidate(
        const ::webrtc::IceCandidateInterface* candidate) {
    // We discovered a candidate, now we just send it over.
    RTC_LOG(INFO) << "participant" << mId
                  << ", candidate: " << candidate->sdp_mline_index();

    std::string sdp;
    if (!candidate->ToString(&sdp)) {
        RTC_LOG(LS_ERROR) << "Failed to serialize candidate";
        return;
    }

    json msg;
    msg["sdpMid"] = candidate->sdp_mid();
    msg["sdpMLineIndex"] = candidate->sdp_mline_index();
    msg["candidate"] = sdp;
    SendMessage(msg);
}

void Participant::SendToBridge(json msg) {
    mSwitchboard->send(Switchboard::BRIDGE_RECEIVER, msg);
}

void Participant::SendMessage(json msg) {
    mSwitchboard->send(mPeerId, msg);
}

void Participant::IncomingMessage(json msg) {
    if (msg.count("candidate")) {
        HandleCandidate(msg["candidate"]);
    }
    if (msg.count("sdp")) {
        HandleOffer(msg["sdp"]);
    }
}

void Participant::HandleCandidate(const json& msg) const {
    if (!msg.count("sdpMid") || !msg.count("sdpMLineIndex") ||
        !msg.count("candidate")) {
        RTC_LOG(WARNING) << "Missing required properties!";
        return;
    }
    std::string sdp_mid = msg["sdpMid"];
    int sdp_mlineindex = msg["sdpMLineIndex"];
    std::string sdp = msg["candidate"];
    ::webrtc::SdpParseError error;

    std::unique_ptr<::webrtc::IceCandidateInterface> candidate(
            CreateIceCandidate(sdp_mid, sdp_mlineindex, sdp, &error));
    if (!candidate.get()) {
        RTC_LOG(WARNING) << "Can't parse received candidate message. "
                         << "SdpParseError was: " << error.description;
        return;
    }
    if (!mPeerConnection->AddIceCandidate(candidate.get())) {
        RTC_LOG(WARNING) << "Failed to apply the received candidate";
        return;
    }
    RTC_LOG(INFO) << " Received candidate :" << msg;
}

void Participant::HandleOffer(const json& msg) const {
    std::string type = msg["type"];
    if (type == "offer-loopback") {
        RTC_LOG(LS_ERROR) << "We are not doing loopbacks";
        return;
    }
    if (!msg.count("sdp")) {
        RTC_LOG(WARNING) << "Message doesn't contain session description";
        return;
    }

    std::string sdp = msg["sdp"];
    ::webrtc::SdpParseError error;
    ::webrtc::SessionDescriptionInterface* session_description(
            CreateSessionDescription(type, sdp, &error));
    if (!session_description) {
        RTC_LOG(WARNING) << "Can't parse received session description message. "
                         << "SdpParseError was: " << error.description;
        return;
    }
    RTC_LOG(INFO) << " Received session description :" << msg;
    mPeerConnection->SetRemoteDescription(
            DummySetSessionDescriptionObserver::Create(), session_description);
    if (session_description->type() ==
        ::webrtc::SessionDescriptionInterface::kOffer) {
        mPeerConnection->CreateAnswer(
                (::webrtc::CreateSessionDescriptionObserver*)this,
                ::webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
    }
    return;
}

void Participant::OnSuccess(::webrtc::SessionDescriptionInterface* desc) {
    mPeerConnection->SetLocalDescription(
            DummySetSessionDescriptionObserver::Create(), desc);

    std::string sdp;
    desc->ToString(&sdp);

    json jmessage;
    jmessage["type"] = desc->type();
    jmessage["sdp"] = sdp;
    SendMessage(jmessage);
}

void Participant::OnFailure(const std::string& error) {
    RTC_LOG(LERROR) << error;
}

void Participant::OnFailure(::webrtc::RTCError error) {
    RTC_LOG(LERROR) << ToString(error.type()) << ": " << error.message();
}

cricket::VideoCapturer* Participant::OpenVideoCaptureDevice(
        const std::string& handle) {
    videocapturemodule::VideoShareFactory* factory =
            new videocapturemodule::VideoShareFactory(handle, kFps);
    std::unique_ptr<cricket::WebRtcVideoCapturer> capturer(
            new cricket::WebRtcVideoCapturer(factory));
    cricket::Device default_device(handle, 0);
    if (!capturer->Init(default_device)) {
        RTC_LOG(LERROR) << "unable to initialize device on handle: " << handle;
        return nullptr;
    }
    return capturer.release();
}

bool Participant::AddVideoTrack(const std::string& handle) {
    std::unique_ptr<cricket::VideoCapturer> device(
            OpenVideoCaptureDevice(handle));
    if (!device) {
        return false;
    }

    if (mActiveVideoTracks.count(handle) != 0) {
        RTC_LOG(LS_ERROR) << "Track " << handle
                          << " already active, not adding again.";
    }

    scoped_refptr<::webrtc::VideoTrackInterface> video_track(
            mPeerConnectionFactory->CreateVideoTrack(
                    handle, mPeerConnectionFactory->CreateVideoSource(
                                    std::move(device), nullptr)));

    auto result = mPeerConnection->AddTrack(video_track, {handle});
    if (!result.ok()) {
        RTC_LOG(LS_ERROR) << "Failed to add track to video: "
                          << result.error().message();
        return false;
    }

    mActiveVideoTracks[handle] = result.value();
    return true;
}

void Participant::CreateOffer() {
    RTC_LOG(INFO) << "Creating offer";
    mPeerConnection->CreateOffer(
            this, ::webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
}

bool Participant::RemoveVideoTrack(const std::string& handle) {
    if (mActiveVideoTracks.count(handle) == 0) {
        RTC_LOG(LS_ERROR) << "Track " << handle
                          << " not active, no need to remove.";
        return false;
    }

    auto track = mActiveVideoTracks[handle];
    mPeerConnection->RemoveTrack(track);
    return true;
}

void Participant::AddDataChannel(const std::string& label) {
    auto channel = mPeerConnection->CreateDataChannel(label, nullptr);
    mEventForwarders[label] =
            std::make_unique<EventForwarder>(this, channel, label);
}

bool Participant::Initialize() {
    if (!CreatePeerConnection(DTLS_ON)) {
        RTC_LOG(LS_ERROR) << "Unable to create peer connection";
        mPeerConnection = nullptr;
    }

    if (mPeerConnection.get() == nullptr) {
        RTC_LOG(LS_ERROR) << "Failed to get peer connection";
        return false;
    }

    AddDataChannel("mouse");
    AddDataChannel("touch");
    AddDataChannel("keyboard");
    return true;
}

bool Participant::CreatePeerConnection(bool dtls) {
    RTC_DCHECK(mPeerConnection.get() == nullptr);

    ::webrtc::PeerConnectionInterface::RTCConfiguration config;
    ::webrtc::PeerConnectionInterface::IceServer server;
    config.sdp_semantics = ::webrtc::SdpSemantics::kUnifiedPlan;
    config.enable_dtls_srtp = dtls;
    server.uri = kStunUri;
    config.servers.push_back(server);

    mPeerConnection = mPeerConnectionFactory->CreatePeerConnection(
            config, nullptr, nullptr, this);
    return mPeerConnection.get() != nullptr;
}
}  // namespace webrtc
}  // namespace emulator
