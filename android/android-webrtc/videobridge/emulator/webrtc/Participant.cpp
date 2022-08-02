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
#include <functional>
#include <memory>         // for unique_ptr
#include <string>         // for string, hash
#include <unordered_map>  // for unordered_map
#include <utility>        // for pair
#include <vector>         // for vector

#include "android/emulation/control/utils/EmulatorGrcpClient.h"             // for EmulatorGrpc...
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
#include "emulator/webrtc/RtcConfig.h"

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


const std::string Participant::kAudioTrack = "grpcAudioTrack";
const std::string Participant::kVideoTrack = "grpcDisplay-";

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

static std::string asString(
        ::webrtc::PeerConnectionInterface::IceConnectionState value) {
    std::string s = "";
#define LABEL(p)                                                     \
    case (::webrtc::PeerConnectionInterface::IceConnectionState::p): \
        s = #p;                                                      \
        break;
    switch (value) {
        LABEL(kIceConnectionNew)
        LABEL(kIceConnectionChecking)
        LABEL(kIceConnectionConnected)
        LABEL(kIceConnectionCompleted)
        LABEL(kIceConnectionFailed)
        LABEL(kIceConnectionDisconnected)
        LABEL(kIceConnectionClosed)
        LABEL(kIceConnectionMax)
    }
#undef LABEL
    return s;
}

static std::string asString(
        ::webrtc::PeerConnectionInterface::PeerConnectionState value) {
    std::string s = "";
#define LABEL(p)                                                      \
    case (::webrtc::PeerConnectionInterface::PeerConnectionState::p): \
        s = #p;                                                       \
        break;
    switch (value) {
        LABEL(kNew)
        LABEL(kConnecting)
        LABEL(kConnected)
        LABEL(kDisconnected)
        LABEL(kFailed)
        LABEL(kClosed)
    }
#undef LABEL
    return s;
}

void logRtcError(::webrtc::RTCError error) {
    if (!error.ok())
        RTC_LOG(LS_ERROR) << error.message();
}

class SetRemoteDescriptionCallback
    : public rtc::RefCountedObject<
              ::webrtc::SetRemoteDescriptionObserverInterface> {
public:
    SetRemoteDescriptionCallback() {}
    void OnSetRemoteDescriptionComplete(::webrtc::RTCError error) override {
        logRtcError(error);
    }

private:
    DISALLOW_COPY_AND_ASSIGN(SetRemoteDescriptionCallback);
};

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
    DISALLOW_COPY_AND_ASSIGN(DummySetSessionDescriptionObserver);
};

class DefaultSessionDescriptionObserver
    : public ::webrtc::CreateSessionDescriptionObserver {
public:
    using ResultCallback =
            std::function<void(::webrtc::SessionDescriptionInterface*)>;

    void OnSuccess(::webrtc::SessionDescriptionInterface* desc) override {
        mCallback(desc);
    }
    void OnFailure(::webrtc::RTCError error) override { logRtcError(error); }

    static DefaultSessionDescriptionObserver* Create(
            const ResultCallback& callback) {
        return new rtc::RefCountedObject<DefaultSessionDescriptionObserver>(
                callback);
    }

protected:
    explicit DefaultSessionDescriptionObserver(const ResultCallback& callback)
        : mCallback(callback) {}

private:
    ResultCallback mCallback;
    DISALLOW_COPY_AND_ASSIGN(DefaultSessionDescriptionObserver);
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
        mEmulatorGrpc = mClient->stub<android::emulation::control::EmulatorController>();
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

Participant::Participant(RtcConnection* board,
                         std::string id,
                         json rtcConfig,
                         VideoTrackReceiver* vtr)
    : mRtcConnection(board),
      mPeerId(id),
      mRtcConfig(std::move(rtcConfig)),
      mVideoReceiver(vtr) {}

Participant::~Participant() {
    RTC_LOG(INFO) << "Participant " << mPeerId << ", completed.";
    if (mLocalAdbSocket) {
        // There's a corner case where we never established a local adb
        // connection and we could be spinning in a connect loop.
        mLocalAdbSocket->close();
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

using ::webrtc::PeerConnectionInterface;

void Participant::OnIceConnectionChange(
        PeerConnectionInterface::IceConnectionState new_state) {
    RTC_LOG(INFO) << "OnIceConnectionChange: Participant: " << mPeerId << ": "
                  << asString(new_state);
}

void Participant::OnConnectionChange(
        PeerConnectionInterface::PeerConnectionState new_state) {
    RTC_LOG(INFO) << "OnConnectionChange: Participant: " << mPeerId << ": "
                  << asString(new_state);
    bool closed = false;
    switch (new_state) {
        case PeerConnectionInterface::PeerConnectionState::kFailed:
        case PeerConnectionInterface::PeerConnectionState::kClosed:

            // We will only notify closing 1 time.
            if (mClosed.compare_exchange_strong(closed, true)) {
                mRtcConnection->rtcConnectionClosed(mPeerId);
            }
            break;
        default: /* nop */
            break;
    }
}

void Participant::DoClose() {
    RTC_LOG(INFO) << "Closing peer connection.";
    std::unique_lock<std::mutex> lock(mClosedMutex);
    if (mPeerConnection) {
        mPeerConnection->Close();
    }

    mPeerConnection = nullptr;
    mAudioSource = nullptr;
    mVideoSources.clear();
    mEventForwarders.clear();
    mDataChannels.clear();
    mActiveTracks.clear();

    mCvClosed.notify_all();
}

void Participant::Close() {
    // Closing has to happen on the signaling thread.
    if (mRtcConnection->signalingThread()->IsCurrent()) {
        DoClose();
    } else {
        mRtcConnection->signalingThread()->Invoke<void>(RTC_FROM_HERE,
                                                        [&] { DoClose(); });
    }
}
void Participant::WaitForClose() {
    std::unique_lock<std::mutex> lock(mClosedMutex);
    mCvClosed.wait(lock, [&] { return mPeerConnection == nullptr; });
}

void Participant::OnIceCandidate(
        const ::webrtc::IceCandidateInterface* candidate) {
    // We discovered a candidate, now we just send it over.
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

void Participant::SendMessage(json msg) {
    RTC_LOG(INFO) << "Send: " << mPeerId << ", :" << msg;
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
    RTC_LOG(INFO) << "IncomingMessage: " << msg.dump();
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

void Participant::HandleCandidate(const json& msg) {
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
    if (!candidate) {
        RTC_LOG(WARNING) << "Can't parse received candidate message. "
                         << "SdpParseError was: " << error.description;
        return;
    }
    mPeerConnection->AddIceCandidate(std::move(candidate), &logRtcError);
}

void Participant::HandleOffer(const json& msg) {
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
    std::unique_ptr<::webrtc::SessionDescriptionInterface> session_description(
            CreateSessionDescription(type, sdp, &error));
    if (!session_description) {
        RTC_LOG(WARNING) << "Can't parse received session description message. "
                         << "SdpParseError was: " << error.description;
        return;
    }
    auto sort = session_description->type();
    mPeerConnection->SetRemoteDescription(std::move(session_description),
                                          new SetRemoteDescriptionCallback());
    if (sort == ::webrtc::SessionDescriptionInterface::kOffer) {
        mPeerConnection->CreateAnswer(
                DefaultSessionDescriptionObserver::Create(
                        [&](auto desc) { ReceivedSessionDescription(desc); }),
                {});
    }
}

void Participant::ReceivedSessionDescription(
        ::webrtc::SessionDescriptionInterface* desc) {
    mPeerConnection->SetLocalDescription(
            DummySetSessionDescriptionObserver::Create(), desc);

    std::string sdp;
    desc->ToString(&sdp);

    json jmessage;
    jmessage["type"] = desc->type();
    jmessage["sdp"] = sdp;
    SendMessage(jmessage);
}

bool Participant::AddVideoTrack(int displayId) {
    std::string handle = kVideoTrack + std::to_string(displayId);
    if (mActiveTracks.count(handle) != 0) {
        RTC_LOG(LS_ERROR) << "Track " << handle
                          << " already active, not adding again.";
    }
    RTC_LOG(INFO) << "Adding video track: [" << handle << "]";
    auto track =
            mRtcConnection->getMediaSourceLibrary()->getVideoSource(displayId);
    scoped_refptr<::webrtc::VideoTrackInterface> video_track(
            mRtcConnection->getPeerConnectionFactory()->CreateVideoTrack(
                    handle, track));

    auto result = mPeerConnection->AddTrack(video_track, {handle});
    if (!result.ok()) {
        RTC_LOG(LS_ERROR) << "Failed to add track to video: "
                          << result.error().message();
        return false;
    }
    mVideoSources.push_back(track);
    mActiveTracks[handle] = result.value();
    return true;
}

bool Participant::AddAudioTrack(const std::string& audioDumpFile) {
    if (mActiveTracks.count(kAudioTrack) != 0) {
        RTC_LOG(LS_ERROR) << "Track " << kAudioTrack
                          << " already active, not adding again.";
    }
    RTC_LOG(INFO) << "Adding audio track: [" << kAudioTrack << "]";
    auto track = mRtcConnection->getMediaSourceLibrary()->getAudioSource();
    track->setAudioDumpFile(audioDumpFile);

    scoped_refptr<::webrtc::AudioTrackInterface> audio_track(
            mRtcConnection->getPeerConnectionFactory()->CreateAudioTrack(
                    kAudioTrack, track));

    auto result = mPeerConnection->AddTrack(audio_track, {kAudioTrack});
    if (!result.ok()) {
        RTC_LOG(LS_ERROR) << "Failed to add audio track: "
                          << result.error().message();
        return false;
    }
    mActiveTracks[kAudioTrack] = result.value();
    mAudioSource = track;
    return true;
}

void Participant::CreateOffer() {
    mPeerConnection->CreateOffer(
            DefaultSessionDescriptionObserver::Create(
                    [&](auto desc) { ReceivedSessionDescription(desc); }),
            {});
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
    mEventForwarders[label] = std::make_unique<EventForwarder>(
            mRtcConnection->getEmulatorClient(), channel, label);
}

bool Participant::Initialize() {
    bool success = CreatePeerConnection(mRtcConfig);
    if (success) {
        AddDataChannel(DataChannelLabel::mouse);
        AddDataChannel(DataChannelLabel::touch);
        AddDataChannel(DataChannelLabel::keyboard);
    }
    return success;
}

bool Participant::CreatePeerConnection(const json& rtcConfiguration) {
    RTC_DCHECK(mPeerConnection.get() == nullptr);
    mPeerConnection =
            mRtcConnection->getPeerConnectionFactory()->CreatePeerConnection(
                   RtcConfig::parse(rtcConfiguration),
                    ::webrtc::PeerConnectionDependencies(this));
    return mPeerConnection.get() != nullptr;
}

}  // namespace webrtc
}  // namespace emulator
