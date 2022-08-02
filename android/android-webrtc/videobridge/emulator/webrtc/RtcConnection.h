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
#include <api/peer_connection_interface.h>  // for PeerC...
#include <api/scoped_refptr.h>              // for scope...
#include <rtc_base/thread.h>                // for Thread
#include <memory>                           // for uniqu...
#include <nlohmann/json.hpp>                // for json
#include <string>                           // for string

#include "android/base/async/DefaultLooper.h"                   // for Defau...
#include "android/base/async/RecurrentTask.h"                   // for Recur...
#include "emulator/webrtc/capture/GoldfishAudioDeviceModule.h"  // for Goldf...
#include "emulator/webrtc/capture/MediaSourceLibrary.h"

namespace emulator {
namespace webrtc {

using json = nlohmann::json;
using android::base::Looper;
using android::base::RecurrentTask;

// An RtcConnection represents a connection of a webrtc endpoint.
// The RtcConnection is used by a peerconnection to send jsep messages
// to another peerconnection.å
class RtcConnection {
public:
    RtcConnection(EmulatorGrpcClient* client);
    ~RtcConnection();

    // Called when a participant is unable to continue the rtc stream.
    // The participant will no longer be in use. Usually you want to
    // close the participant on the signaling thread after which
    // it can be safely removed.
    virtual void rtcConnectionClosed(const std::string participant) = 0;

    // Send a jsep json message to the participant identified by to.
    virtual void send(std::string to, json msg) = 0;

    // Gets an android based looper that can be used for async operations.
    Looper* getLooper() { return &mLooper; }

    // gRPC connection to the emulator.
    EmulatorGrpcClient* getEmulatorClient() { return mClient; }

    // The WebRTC peerconnection factory.
    ::webrtc::PeerConnectionFactoryInterface* getPeerConnectionFactory() {
        return mConnectionFactory.get();
    }

    // The library used to obtain audio/video sources from the emulator.
    MediaSourceLibrary* getMediaSourceLibrary() { return mLibrary.get(); }

    // The webrtc signalling thread. Use this if you are operating on peerconnections.
    rtc::Thread* signalingThread() { return mSignaling.get(); }

private:
    EmulatorGrpcClient* mClient;
    GoldfishAudioDeviceModule mGoldfishAdm;
    std::unique_ptr<MediaSourceLibrary> mLibrary;

    // PeerConnection factory and threads.
    std::unique_ptr<::webrtc::TaskQueueFactory> mTaskFactory;
    std::unique_ptr<rtc::Thread> mWorker;
    std::unique_ptr<rtc::Thread> mSignaling;
    std::unique_ptr<rtc::Thread> mNetwork;
    rtc::scoped_refptr<::webrtc::PeerConnectionFactoryInterface>
            mConnectionFactory;
    // Looper
    android::base::DefaultLooper mLooper;
    std::unique_ptr<std::thread> mLooperThread{nullptr};
    bool mLooperActive{true};
    RecurrentTask mKeepAlive;
};
}  // namespace webrtc
}  // namespace emulator
