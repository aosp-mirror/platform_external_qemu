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
#include "emulator/webrtc/Participant.h"  // for Participant

#include <api/data_channel_interface.h>        // for DataChannelI...
#include <api/jsep.h>                          // for IceCandidate...
#include <api/media_stream_interface.h>        // for MediaStreamT...
#include <api/peer_connection_interface.h>     // for PeerConnecti...
#include <api/rtc_error.h>                     // for RTCError
#include <api/rtp_sender_interface.h>          // for RtpSenderInt...
#include <api/scoped_refptr.h>                 // for scoped_refptr
#include <api/video/video_source_interface.h>  // for VideoSinkWants
#include <grpcpp/grpcpp.h>                     // for ClientContext
#include <rtc_base/checks.h>                   // for FatalLogCall
#include <rtc_base/copy_on_write_buffer.h>     // for CopyOnWriteB...
#include <rtc_base/logging.h>                  // for Val, RTC_LOG
#include <rtc_base/ref_counted_object.h>       // for RefCountedOb...
#include <stdint.h>                            // for uint8_t, uin...
#include <memory>                              // for unique_ptr
#include <string>                              // for string, hash
#include <unordered_map>                       // for unordered_map
#include <utility>                             // for pair
#include <vector>                              // for vector

#include "emulator/net/EmulatorGrcpClient.h"             // for EmulatorGrpc...
#include "emulator/net/SocketForwarder.h"                // for AdbPortForwa...
#include "emulator/webrtc/RtcConnection.h"               // for json, RtcCon...
#include "emulator/webrtc/Switchboard.h"                 // for Switchboard
#include "emulator/webrtc/capture/GrpcAudioSource.h"     // for GrpcAudioSource
#include "emulator/webrtc/capture/GrpcVideoSource.h"     // for GrpcVideoSource
#include "emulator/webrtc/capture/VideoTrackReceiver.h"  // for VideoTrackRe...
#include "emulator_controller.grpc.pb.h"                 // for EmulatorCont...
#include "emulator_controller.pb.h"                      // for KeyboardEvent
#include "google/protobuf/empty.pb.h"                    // for Empty
#include "nlohmann/json.hpp"                             // for basic_json

namespace android {
namespace base {
class AsyncSocket;
}  // namespace base
}  // namespace android

using json = nlohmann::json;
using MediaStreamPair =
        std::pair<std::string, scoped_refptr<webrtc::MediaStreamInterface>>;

namespace emulator {
namespace webrtc {

const std::string Participant::kStunUri = "stun:stun.l.google.com:19302";

static std::string asString(DataChannelLabel value) {
    std::string s = "";
#define LABEL(p)                \
    case (DataChannelLabel::p): \
        s = #p;                 \
        break;
    switch (value) {
        LABEL(adb)
        LABEL(mouse)
        LABEL(keyboard)
        LABEL(touch)
    }
#undef LABEL
    return s;
}

class DummySetSessionDescriptionObserver
    : public ::webrtc::SetSessionDescriptionObserver {
public:
    static DummySetSessionDescriptionObserver* Create() {
        return new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
    }
    virtual void OnSuccess() {}
    virtual void OnFailure(::webrtc::RTCError error){};

protected:
    DummySetSessionDescriptionObserver() {}
    ~DummySetSessionDescriptionObserver() {}
};

EventForwarder::EventForwarder(
        EmulatorGrpcClient* client,
        scoped_refptr<::webrtc::DataChannelInterface> channel,
        DataChannelLabel label)
    : mClient(client), mChannel(channel), mLabel(label) {
    channel->RegisterObserver(this);
}

EventForwarder::~EventForwarder() {
    mChannel->UnregisterObserver();
}

void EventForwarder::OnStateChange() {}

void EventForwarder::OnMessage(const ::webrtc::DataBuffer& buffer) {
    if (!mEmulatorGrpc) {
        mEmulatorGrpc = mClient->stub();
    }
    ::google::protobuf::Empty nullResponse;
    std::unique_ptr<grpc::ClientContext> context = mClient->newContext();
    switch (mLabel) {
        case DataChannelLabel::keyboard: {
            android::emulation::control::KeyboardEvent keyEvent;
            keyEvent.ParseFromArray(buffer.data.data(), buffer.size());
            mEmulatorGrpc->sendKey(context.get(), keyEvent, &nullResponse);
            break;
        }
        case DataChannelLabel::mouse: {
            android::emulation::control::MouseEvent mouseEvent;
            mouseEvent.ParseFromArray(buffer.data.data(), buffer.size());
            mEmulatorGrpc->sendMouse(context.get(), mouseEvent, &nullResponse);
            break;
        }
        case DataChannelLabel::touch: {
            android::emulation::control::TouchEvent touchEvent;
            touchEvent.ParseFromArray(buffer.data.data(), buffer.size());
            mEmulatorGrpc->sendTouch(context.get(), touchEvent, &nullResponse);
            break;
        }
        default:
            break;
    }
}

static std::atomic<uint32_t> sId{0};

Participant::Participant(RtcConnection* board, std::string id, VideoTrackReceiver* vtr)
    : mRtcConnection(board), mPeerId(id), mVideoReceiver(vtr) {
    mId = sId++;
}

Participant::~Participant() {
    RTC_LOG(INFO) << "Participant " << mId << ", completed.";
    if (mPeerConnection) {
        mPeerConnection->Close();
    }
}


void Participant::OnAddStream(
        rtc::scoped_refptr<::webrtc::MediaStreamInterface> stream) {
    if (mVideoReceiver == nullptr) {
        RTC_LOG(WARNING) << "No receivers available!";
        return;
    }
    auto tracks = stream->GetVideoTracks();
    for (const auto& track : tracks) {
        // TODO(jansene): Here we should include multi display settings.
        RTC_LOG(INFO) << "Connecting track: " << track->id();
        track->set_enabled(true);
        track->AddOrUpdateSink(mVideoReceiver, rtc::VideoSinkWants());
    }
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
            mRtcConnection->rtcConnectionDropped(mPeerId);
            break;
        case ::webrtc::PeerConnectionInterface::IceConnectionState::
                kIceConnectionClosed:
            // Boo! we are closed, peer connection will be garbage collected on
            // other thread, as we cannot make ourself invalid on the calling
            // thread that is actually using us.
            RTC_LOG(INFO) << "Closed! Time to erase peer: " << mId;
            mRtcConnection->rtcConnectionClosed(mPeerId);
            break;
        default:;  // Nothing ..
    }
}

void Participant::Close() {
    if (mPeerConnection) {
        RTC_LOG(INFO) << "Closing peer connection.";
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
    mRtcConnection->send(Switchboard::BRIDGE_RECEIVER, msg);
}

void Participant::SendMessage(json msg) {
    mRtcConnection->send(mPeerId, msg);
}

rtc::scoped_refptr<::webrtc::DataChannelInterface> Participant::GetDataChannel(
        DataChannelLabel label) {
    auto fwd = mDataChannels.find(asString(label));
    if (fwd != mDataChannels.end()) {
        return fwd->second;
    }
    return nullptr;
}

void Participant::SendToDataChannel(DataChannelLabel label, std::string data) {
    auto fwd = mDataChannels.find(asString(label));
    if (fwd != mDataChannels.end()) {
        auto buffer =
                ::webrtc::DataBuffer(::rtc::CopyOnWriteBuffer(data), true);
        fwd->second->Send(buffer);
    } else {
        RTC_LOG(INFO) << "Dropping message to " << asString(label)
                      << ", no such datachannel present";
    }
}

void Participant::IncomingMessage(json msg) {
    RTC_LOG(INFO) << "IncomingMessage: " << msg.dump(4);
    if (msg.count("candidate")) {
        if (msg["candidate"].count("candidate")) {
            // Old JS clients wrap this message.
            HandleCandidate(msg["candidate"]);
        } else {
            HandleCandidate(msg);
        }
    }
    if (msg.count("start")) {
        // Initialize the peer connection
        CreatePeerConnection(msg["start"]);
    }
    if (msg.count("sdp")) {
        if (msg["sdp"].count("sdp")) {
            // Old JS clients wrap this message.
            HandleOffer(msg["sdp"]);
        } else {
            HandleOffer(msg);
        }
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

void Participant::OnFailure(::webrtc::RTCError error) {
    RTC_LOG(LERROR) << ToString(error.type()) << ": " << error.message();
}

bool Participant::AddVideoTrack(std::string handle) {
    if (mActiveVideoTracks.count(handle) != 0) {
        RTC_LOG(LS_ERROR) << "Track " << handle
                          << " already active, not adding again.";
    }

    // TODO(jansene): Video sources should be shared amongst participants.
    RTC_LOG(INFO) << "Adding video track: [" << handle << "]";
    auto track = new rtc::RefCountedObject<emulator::webrtc::GrpcVideoSource>(
            mRtcConnection->getEmulatorClient());
    scoped_refptr<::webrtc::VideoTrackInterface> video_track(
            mRtcConnection->getPeerConnectionFactory()->CreateVideoTrack(handle, track));

    auto result = mPeerConnection->AddTrack(video_track, {handle});
    if (!result.ok()) {
        RTC_LOG(LS_ERROR) << "Failed to add track to video: "
                          << result.error().message();
        return false;
    }

    mActiveVideoTracks[handle] = result.value();
    return true;
}

bool Participant::AddAudioTrack(std::string grpc) {
    if (mActiveAudioTracks.count(grpc) != 0) {
        RTC_LOG(LS_ERROR) << "Track " << grpc
                          << " already active, not adding again.";
    }

    // TODO(jansene): Audio sources should be shared amongst participants.
    RTC_LOG(INFO) << "Adding audio track: [" << grpc << "]";
    auto track = new rtc::RefCountedObject<emulator::webrtc::GrpcAudioSource>(
            mRtcConnection->getEmulatorClient());
    scoped_refptr<::webrtc::AudioTrackInterface> audio_track(
            mRtcConnection->getPeerConnectionFactory()->CreateAudioTrack(grpc, track));

    auto result = mPeerConnection->AddTrack(audio_track, {grpc});
    if (!result.ok()) {
        RTC_LOG(LS_ERROR) << "Failed to add audio track: "
                          << result.error().message();
        return false;
    }

    mActiveAudioTracks[grpc] = result.value();
    return true;
}

void Participant::CreateOffer() {
    mPeerConnection->CreateOffer(
            this, ::webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
}

bool Participant::RemoveAudioTrack(std::string grpc) {
    if (mActiveAudioTracks.count(grpc) == 0) {
        RTC_LOG(LS_ERROR) << "Track " << grpc
                          << " not active, no need to remove.";
        return false;
    }

    auto track = mActiveAudioTracks[grpc];
    mPeerConnection->RemoveTrack(track);
    return true;
}

bool Participant::RemoveVideoTrack(std::string handle) {
    if (mActiveVideoTracks.count(handle) == 0) {
        RTC_LOG(LS_ERROR) << "Track " << handle
                          << " not active, no need to remove.";
        return false;
    }

    auto track = mActiveVideoTracks[handle];
    mPeerConnection->RemoveTrack(track);
    return true;
}

void Participant::OnDataChannel(
        rtc::scoped_refptr<::webrtc::DataChannelInterface> channel) {
    RTC_LOG(INFO) << "Registering data channel: " << channel->label();
    if (channel->label() == "adb" && mLocalAdbSocket) {
        mSocketForwarder =
                std::make_unique<SocketForwarder>(mLocalAdbSocket, channel);
    }
    mDataChannels[channel->label()] = channel;
}

bool Participant::RegisterLocalAdbForwarder(
        std::shared_ptr<android::base::AsyncSocket> adbSocket) {
    RTC_LOG(INFO) << "Registering adb forwarding channel.";
    mLocalAdbSocket = adbSocket;
    auto channel = mPeerConnection->CreateDataChannel("adb", nullptr);
    if (!channel) {
        return false;
    }
    mSocketForwarder = std::make_unique<SocketForwarder>(adbSocket, channel);
    return true;
}

void Participant::AddDataChannel(const DataChannelLabel label) {
    auto channel = mPeerConnection->CreateDataChannel(asString(label), nullptr);
    mEventForwarders[label] =
            std::make_unique<EventForwarder>(mRtcConnection->getEmulatorClient(), channel, label);
}

bool Participant::Initialize() {
    bool success = CreatePeerConnection({});
    if (success) {
        AddDataChannel(DataChannelLabel::mouse);
        AddDataChannel(DataChannelLabel::touch);
        AddDataChannel(DataChannelLabel::keyboard);
    }
    return success;
}

static ::webrtc::PeerConnectionInterface::IceServer parseIce(json desc) {
    ::webrtc::PeerConnectionInterface::IceServer ice;
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
                ice.urls.push_back(uri.get<std::string>());
            }
        }
    }
    return ice;
}

static ::webrtc::PeerConnectionInterface::RTCConfiguration
parseRTCConfiguration(json rtcConfiguration) {
    ::webrtc::PeerConnectionInterface::RTCConfiguration configuration;
    // TODO(jansene) handle additional properties?
    // https://developer.mozilla.org/en-US/docs/Web/API/RTCPeerConnection/RTCPeerConnection#rtcconfiguration_dictionary
    if (rtcConfiguration.count("iceServers")) {
        auto iceServers = rtcConfiguration["iceServers"];
        for (const auto& iceServer : iceServers) {
            configuration.servers.push_back(parseIce(iceServer));
        }
    }

    // Modern webrtc with multiple audio & video channels.
    configuration.sdp_semantics = ::webrtc::SdpSemantics::kUnifiedPlan;

    // Let's add at least a default stun server if none is present.
    if (configuration.servers.empty()) {
        ::webrtc::PeerConnectionInterface::IceServer server;
        server.uri = Participant::kStunUri;
        configuration.servers.push_back(server);
    }
    return configuration;
}

bool Participant::CreatePeerConnection(json rtcConfiguration) {
    RTC_DCHECK(mPeerConnection.get() == nullptr);

    mPeerConnection = mRtcConnection->getPeerConnectionFactory()->CreatePeerConnection(
            parseRTCConfiguration(rtcConfiguration),
            ::webrtc::PeerConnectionDependencies(this));
    return mPeerConnection.get() != nullptr;
}

}  // namespace webrtc
}  // namespace emulator
