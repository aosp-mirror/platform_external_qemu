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

#include <api/create_peerconnection_factory.h>
#include <api/test/fakeconstraints.h>
#include <emulator/webrtc/capture/VideoShareFactory.h>
#include <rtc_base/logging.h>
#include "rtc_base/third_party/base64/base64.h"

#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "nlohmann/json.hpp"

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
    worker_thread_->Stop();
    signaling_thread_->Stop();
    network_thread_->Stop();
}

Participant::Participant(Switchboard* board,
                         std::string id,
                         std::string mem_handle,
                         int32_t fps)
    : mSwitchboard(board), mPeerId(id), mMemoryHandle(mem_handle), mFps(fps) {}

void Participant::OnIceConnectionChange(
        ::webrtc::PeerConnectionInterface::IceConnectionState new_state) {
    RTC_LOG(INFO) << "OnIceConnectionChange: " << new_state;
    if (new_state == ::webrtc::PeerConnectionInterface::IceConnectionState::
                             kIceConnectionClosed ||
        new_state == ::webrtc::PeerConnectionInterface::IceConnectionState::
                             kIceConnectionFailed) {
        // Boo! we are dead..
        RTC_LOG(INFO) << "Disconnected! Time to clean up";
        mSwitchboard->rtcConnectionDropped(mPeerId);
    }
}

void Participant::OnIceCandidate(
        const ::webrtc::IceCandidateInterface* candidate) {
    // We discovered a candidate, now we just send it over.
    RTC_LOG(INFO) << "candidate: " << candidate->sdp_mline_index();

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
    if (!peer_connection_->AddIceCandidate(candidate.get())) {
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
    peer_connection_->SetRemoteDescription(
            DummySetSessionDescriptionObserver::Create(), session_description);
    if (session_description->type() ==
        ::webrtc::SessionDescriptionInterface::kOffer) {
        peer_connection_->CreateAnswer(
                (::webrtc::CreateSessionDescriptionObserver*)this,
                ::webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
    }
    return;
}

void Participant::OnSuccess(::webrtc::SessionDescriptionInterface* desc) {
    peer_connection_->SetLocalDescription(
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

cricket::VideoCapturer* Participant::OpenVideoCaptureDevice() {
    videocapturemodule::VideoShareFactory* factory =
            new videocapturemodule::VideoShareFactory(mMemoryHandle, mFps);
    std::unique_ptr<cricket::WebRtcVideoCapturer> capturer(
            new cricket::WebRtcVideoCapturer(factory));
    cricket::Device default_device(mMemoryHandle, 0);
    if (!capturer->Init(default_device)) {
        RTC_LOG(LERROR) << "unable to initialize device on handle: "
                        << mMemoryHandle;
        return nullptr;
    }
    return capturer.release();
}

bool Participant::AddStreams() {
    if (!peer_connection_->GetSenders().empty()) {
        return true;  // Already added tracks.
    }
    // Well, we need audio at some point..
    std::unique_ptr<cricket::VideoCapturer> device(OpenVideoCaptureDevice());
    if (!device) {
        return false;
    }

    scoped_refptr<::webrtc::VideoTrackInterface> video_track(
            peer_connection_factory_->CreateVideoTrack(
                    kVideoLabel, peer_connection_factory_->CreateVideoSource(
                                         std::move(device), nullptr)));

    auto result = peer_connection_->AddTrack(video_track, {kStreamLabel});
    if (!result.ok()) {
        RTC_LOG(LS_ERROR) << "Failed to add track to video: "
                          << result.error().message();
        return false;
    }
    return true;
}

void Participant::AddDataChannel(const std::string& label) {
    auto channel = peer_connection_->CreateDataChannel(label, nullptr);
    mEventForwarders[label] =
            std::make_unique<EventForwarder>(this, channel, label);
}

bool Participant::Initialize() {
    RTC_DCHECK(peer_connection_factory_.get() == nullptr);
    RTC_DCHECK(peer_connection_.get() == nullptr);

    network_thread_ = rtc::Thread::CreateWithSocketServer();
    network_thread_->Start();
    worker_thread_ = rtc::Thread::Create();
    worker_thread_->Start();
    signaling_thread_ = rtc::Thread::Create();
    signaling_thread_->Start();

    peer_connection_factory_ = ::webrtc::CreatePeerConnectionFactory(
            network_thread_.get() /* network_thread */,
            worker_thread_.get() /* worker_thread */,
            signaling_thread_.get() /* signaling_thread */,
            nullptr /* default_adm */,
            ::webrtc::CreateBuiltinAudioEncoderFactory(),
            ::webrtc::CreateBuiltinAudioDecoderFactory(),
            ::webrtc::CreateBuiltinVideoEncoderFactory(),
            ::webrtc::CreateBuiltinVideoDecoderFactory(),
            nullptr /* audio_mixer */, nullptr /* audio_processing */);

    if (!peer_connection_factory_.get()) {
        RTC_LOG(LS_ERROR) << "Unable to configure peer connection factory";
        DeletePeerConnection();
        return false;
    }

    if (!CreatePeerConnection(DTLS_ON)) {
        RTC_LOG(LS_ERROR) << "Unable to create peer connection";
        DeletePeerConnection();
    }

    if (!AddStreams()) {
        RTC_LOG(LS_ERROR) << "Failed to add streams";
        DeletePeerConnection();
        return false;
    }

    if (peer_connection_.get() == nullptr) {
        RTC_LOG(LS_ERROR) << "Failed to get peer connection";
        return false;
    }

    AddDataChannel("mouse");
    AddDataChannel("touch");
    AddDataChannel("keyboard");

    RTC_LOG(INFO) << "Creating offer";
    peer_connection_->CreateOffer(
            this, ::webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
    return true;
}

void Participant::DeletePeerConnection() {
    peer_connection_ = nullptr;
    peer_connection_factory_ = nullptr;
}

bool Participant::CreatePeerConnection(bool dtls) {
    RTC_DCHECK(peer_connection_factory_.get() != nullptr);
    RTC_DCHECK(peer_connection_.get() == nullptr);

    ::webrtc::PeerConnectionInterface::RTCConfiguration config;
    ::webrtc::PeerConnectionInterface::IceServer server;
    config.sdp_semantics = ::webrtc::SdpSemantics::kUnifiedPlan;
    config.enable_dtls_srtp = dtls;
    server.uri = kStunUri;
    config.servers.push_back(server);

    peer_connection_ = peer_connection_factory_->CreatePeerConnection(
            config, nullptr, nullptr, this);
    return peer_connection_.get() != nullptr;
}
}  // namespace webrtc
}  // namespace emulator
