#pragma once

#include <api/jsep.h>
#include <api/peerconnectioninterface.h>
#include <emulator/net/SocketTransport.h>

#include "Switchboard.h"
#include "nlohmann/json.hpp"

using cricket::VideoCapturer;
using json = nlohmann::json;
using rtc::scoped_refptr;
using webrtc::PeerConnectionInterface;
using webrtc::VideoTrackInterface;

namespace emulator {
namespace webrtc {

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

// A Participant in an webrtc streaming session. This class is
// repsonsbile for:
//
// 1. Creating the audio & video streams.
// 2. Do network discovery (ice etc).
// 3. Exchanging of offers between remote client.
class Participant : public EmptyConnectionObserver,
                    public ::webrtc::CreateSessionDescriptionObserver {
public:
    Participant(Switchboard* board, std::string id, std::string mem_handle);
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
    void OnFailure(const std::string& error) override;

    void IncomingMessage(json msg);
    bool Initialize();

private:
    void SendMessage(json msg);
    void HandleOffer(const json& msg) const;
    void HandleCandidate(const json& msg) const;
    void DeletePeerConnection();
    bool AddStreams();
    bool CreatePeerConnection(bool dtls);
    VideoCapturer* OpenVideoCaptureDevice();

    scoped_refptr<PeerConnectionInterface> peer_connection_;
    scoped_refptr<::webrtc::PeerConnectionFactoryInterface>
            peer_connection_factory_;
    std::map<std::string, scoped_refptr<::webrtc::MediaStreamInterface>>
            active_streams_;

    Switchboard* mSwitchboard;
    std::string mPeerId;
    std::string mMemoryHandle;
};
}  // namespace webrtc
}  // namespace emulator
