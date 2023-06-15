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

#include "aemu/base/Log.h"
#include "aemu/base/async/ThreadLooper.h"
#include "aemu/base/misc/StringUtils.h"  // for splitTokens
#include "android/emulation/control/adb/AdbInterface.h"
#include "emulator/webrtc/RtcConfig.h"
#include "emulator/webrtc/RtcConnection.h"  // for json, RtcCon...
#include "emulator/webrtc/Switchboard.h"    // for Switchboard
#include "emulator/webrtc/capture/InprocessVideoSource.h"  // for InprocessVideoSource
#include "emulator/webrtc/capture/VideoTrackReceiver.h"  // for VideoTrackRe...
#include "emulator_controller.pb.h"                      // for KeyboardEvent
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
        LOG(ERROR) << error.message();
}

class SetRemoteDescriptionCallback
    : public rtc::RefCountedObject<
              ::webrtc::SetRemoteDescriptionObserverInterface> {
public:
    SetRemoteDescriptionCallback() {}
    void OnSetRemoteDescriptionComplete(::webrtc::RTCError error) override {
        logRtcError(error);
    }

    static rtc::scoped_refptr<SetRemoteDescriptionCallback> Create() {
        return rtc::scoped_refptr<SetRemoteDescriptionCallback>(
                new rtc::RefCountedObject<SetRemoteDescriptionCallback>());
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
        const AndroidConsoleAgents* agents,
        scoped_refptr<::webrtc::DataChannelInterface> channel,
        DataChannelLabel label)
    : mAgents(agents),
      mKeyEventSender(agents),
      mTouchEventSender(agents),
      mChannel(channel),
      mLabel(label) {
    channel->RegisterObserver(this);
}

EventForwarder::~EventForwarder() {
    mChannel->UnregisterObserver();
}

void EventForwarder::OnStateChange() {}

void EventForwarder::OnMessage(const ::webrtc::DataBuffer& buffer) {
    switch (mLabel) {
        case DataChannelLabel::keyboard: {
            android::emulation::control::KeyboardEvent keyEvent;
            keyEvent.ParseFromArray(buffer.data.data(), buffer.size());
            mKeyEventSender.send(keyEvent);
            break;
        }
        case DataChannelLabel::mouse: {
            android::emulation::control::MouseEvent mouseEvent;
            mouseEvent.ParseFromArray(buffer.data.data(), buffer.size());
            auto agent = mAgents->user_event;
            android::base::ThreadLooper::runOnMainLooper([agent, mouseEvent]() {
                agent->sendMouseEvent(mouseEvent.x(), mouseEvent.y(), 0,
                                      mouseEvent.buttons(),
                                      mouseEvent.display());
            });
            break;
        }
        case DataChannelLabel::touch: {
            android::emulation::control::TouchEvent touchEvent;
            touchEvent.ParseFromArray(buffer.data.data(), buffer.size());
            mTouchEventSender.send(touchEvent);
            break;
        }
        case DataChannelLabel::adb: {
            auto adbInterface = android::emulation::AdbInterface::getGlobal();
            if (!adbInterface) {
                LOG(DEBUG) << "find no adb interface";
                return;
            }
            const char* data =
                    reinterpret_cast<const char*>(buffer.data.data());
            auto adbCmds = android::base::Split(data, " ");
            // Remove "adb"
            adbCmds.erase(adbCmds.begin());
            adbInterface->enqueueCommand(adbCmds);
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
    LOG(INFO) << "Participant " << mPeerId << ", completed.";
}

void Participant::OnAddStream(
        rtc::scoped_refptr<::webrtc::MediaStreamInterface> stream) {
    if (mVideoReceiver == nullptr) {
        LOG(WARNING) << "No receivers available!";
        return;
    }
    auto tracks = stream->GetVideoTracks();
    for (const auto& track : tracks) {
        // TODO(jansene): Here we should include multi display settings.
        LOG(INFO) << "Connecting track: " << track->id();
        track->set_enabled(true);
        track->AddOrUpdateSink(mVideoReceiver, rtc::VideoSinkWants());
    }
}

using ::webrtc::PeerConnectionInterface;

void Participant::OnIceConnectionChange(
        PeerConnectionInterface::IceConnectionState new_state) {
    LOG(INFO) << "OnIceConnectionChange: Participant: " << mPeerId << ": "
              << asString(new_state);
}

void Participant::OnConnectionChange(
        PeerConnectionInterface::PeerConnectionState new_state) {
    LOG(INFO) << "OnConnectionChange: Participant: " << mPeerId << ": "
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
    LOG(INFO) << "Closing peer connection.";
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
        mRtcConnection->signalingThread()->BlockingCall([&] { DoClose(); });
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
        LOG(ERROR) << "Failed to serialize candidate";
        return;
    }

    json msg;
    msg["sdpMid"] = candidate->sdp_mid();
    msg["sdpMLineIndex"] = candidate->sdp_mline_index();
    msg["candidate"] = sdp;
    SendMessage(msg);
}

void Participant::SendMessage(json msg) {
    LOG(INFO) << "Send: " << mPeerId << ", :" << msg;
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
        LOG(INFO) << "Dropping message to " << asString(label)
                  << ", no such datachannel present";
    }
}

void Participant::IncomingMessage(json msg) {
    assert(!msg.is_discarded());
    LOG(INFO) << "IncomingMessage: " << msg.dump();
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
    assert(!msg.is_discarded());
    if (!msg.count("sdpMid") || !msg.count("sdpMLineIndex") ||
        !msg.count("candidate")) {
        LOG(WARNING) << "Missing required properties!";
        return;
    }
    std::string sdp_mid = msg["sdpMid"];
    int sdp_mlineindex = msg["sdpMLineIndex"];
    std::string sdp = msg["candidate"];
    ::webrtc::SdpParseError error;

    std::unique_ptr<::webrtc::IceCandidateInterface> candidate(
            CreateIceCandidate(sdp_mid, sdp_mlineindex, sdp, &error));
    if (!candidate) {
        LOG(WARNING) << "Can't parse received candidate message. "
                     << "SdpParseError was: " << error.description;
        return;
    }
    mPeerConnection->AddIceCandidate(std::move(candidate), &logRtcError);
}

void Participant::HandleOffer(const json& msg) {
    assert(!msg.is_discarded());
    std::string type = msg["type"];
    if (type == "offer-loopback") {
        LOG(ERROR) << "We are not doing loopbacks";
        return;
    }
    if (!msg.count("sdp")) {
        LOG(WARNING) << "Message doesn't contain session description";
        return;
    }

    std::string sdp = msg["sdp"];
    ::webrtc::SdpParseError error;
    std::unique_ptr<::webrtc::SessionDescriptionInterface> session_description(
            CreateSessionDescription(type, sdp, &error));
    if (!session_description) {
        LOG(WARNING) << "Can't parse received session description message. "
                     << "SdpParseError was: " << error.description;
        return;
    }
    auto sort = session_description->type();
    mPeerConnection->SetRemoteDescription(std::move(session_description),
                                          SetRemoteDescriptionCallback::Create());
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
        LOG(ERROR) << "Track " << handle
                   << " already active, not adding again.";
    }
    LOG(INFO) << "Adding video track: [" << handle << "]";
    auto track =
            mRtcConnection->getMediaSourceLibrary()->getVideoSource(displayId);
    scoped_refptr<::webrtc::VideoTrackInterface> video_track(
            mRtcConnection->getPeerConnectionFactory()->CreateVideoTrack(
                    handle, track.get()));

    auto result = mPeerConnection->AddTrack(video_track, {handle});
    if (!result.ok()) {
        LOG(ERROR) << "Failed to add track to video: "
                   << result.error().message();
        return false;
    }
    mVideoSources.push_back(track);
    mActiveTracks[handle] = result.value();
    return true;
}

bool Participant::AddAudioTrack(const std::string& audioDumpFile) {
    if (mActiveTracks.count(kAudioTrack) != 0) {
        LOG(ERROR) << "Track " << kAudioTrack
                   << " already active, not adding again.";
    }
    LOG(INFO) << "Adding audio track: [" << kAudioTrack << "]";
    auto track = mRtcConnection->getMediaSourceLibrary()->getAudioSource();
    track->setAudioDumpFile(audioDumpFile);

    scoped_refptr<::webrtc::AudioTrackInterface> audio_track(
            mRtcConnection->getPeerConnectionFactory()->CreateAudioTrack(
                    kAudioTrack, track.get()));

    auto result = mPeerConnection->AddTrack(audio_track, {kAudioTrack});
    if (!result.ok()) {
        LOG(ERROR) << "Failed to add audio track: " << result.error().message();
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
    LOG(INFO) << "Registering data channel: " << channel->label();
    mDataChannels[channel->label()] = channel;
}

void Participant::AddDataChannel(const DataChannelLabel label) {
    auto channel = mPeerConnection->CreateDataChannel(asString(label), nullptr);
    mEventForwarders[label] = std::make_unique<EventForwarder>(
            getConsoleAgents(), channel, label);
}

bool Participant::Initialize() {
    bool success = CreatePeerConnection(mRtcConfig);
    if (success) {
        AddDataChannel(DataChannelLabel::mouse);
        AddDataChannel(DataChannelLabel::keyboard);
        AddDataChannel(DataChannelLabel::touch);
        AddDataChannel(DataChannelLabel::adb);
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
