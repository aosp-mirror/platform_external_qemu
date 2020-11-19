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
#include "Switchboard.h"

#include <api/audio/audio_mixer.h>                              // for Audio...
#include <api/audio_codecs/builtin_audio_decoder_factory.h>     // for Creat...
#include <api/audio_codecs/builtin_audio_encoder_factory.h>     // for Creat...
#include <api/create_peerconnection_factory.h>                  // for Creat...
#include <api/peer_connection_interface.h>                      // for PeerC...
#include <api/scoped_refptr.h>                                  // for scope...
#include <api/task_queue/default_task_queue_factory.h>          // for Creat...
#include <api/task_queue/task_queue_factory.h>                  // for TaskQ...
#include <api/video_codecs/builtin_video_decoder_factory.h>     // for Creat...
#include <api/video_codecs/builtin_video_encoder_factory.h>     // for Creat...
#include <modules/audio_device/include/audio_device.h>          // for Audio...
#include <modules/audio_processing/include/audio_processing.h>  // for Audio...
#include <rtc_base/critical_section.h>                          // for CritS...
#include <rtc_base/logging.h>                                   // for RTC_LOG
#include <rtc_base/ref_counted_object.h>                        // for RefCo...
#include <rtc_base/thread.h>                                    // for Thread
#include <map>                                                  // for map
#include <memory>                                               // for share...
#include <ostream>                                              // for opera...
#include <string>                                               // for string
#include <unordered_map>                                        // for unord...
#include <utility>                                              // for move
#include <vector>                                               // for vector

#include "Participant.h"                                        // for Parti...
#include "android/base/Log.h"                                   // for LogSt...
#include "android/base/Optional.h"                              // for Optional
#include "android/base/ProcessControl.h"                        // for parse...
#include "android/base/containers/BufferQueue.h"                // for Buffe...
#include "android/base/synchronization/Lock.h"                  // for Lock
#include "android/base/system/System.h"                         // for System
#include "emulator/net/EmulatorGrcpClient.h"                    // for Emula...
#include "emulator/webrtc/Switchboard.h"                        // for Switc...
#include "emulator/webrtc/capture/GoldfishAudioDeviceModule.h"  // for Goldf...
#include "nlohmann/json.hpp"                                    // for opera...

using namespace android::base;

namespace emulator {
namespace webrtc {

std::string Switchboard::BRIDGE_RECEIVER = "WebrtcVideoBridge";

Switchboard::~Switchboard() {}

Switchboard::Switchboard(EmulatorGrpcClient client,
                         const std::string& shmPath,
                         const std::string& turnConfig)
    : mShmPath(shmPath),
      mTaskFactory(::webrtc::CreateDefaultTaskQueueFactory()),
      mNetwork(rtc::Thread::CreateWithSocketServer()),
      mWorker(rtc::Thread::Create()),
      mSignaling(rtc::Thread::Create()),
      mTurnConfig(parseEscapedLaunchString(turnConfig)),
      mClient(client) {
    mNetwork->SetName("Sw-Network", nullptr);
    mNetwork->Start();
    mWorker->SetName("Sw-Worker", nullptr);
    mWorker->Start();
    mSignaling->SetName("Sw-Signaling", nullptr);
    mSignaling->Start();
    mConnectionFactory = ::webrtc::CreatePeerConnectionFactory(
            mNetwork.get() /* network_thread */,
            mWorker.get() /* worker_thread */,
            mSignaling.get() /* signaling_thread */, &mGoldfishAdm,
            ::webrtc::CreateBuiltinAudioEncoderFactory(),
            ::webrtc::CreateBuiltinAudioDecoderFactory(),
            ::webrtc::CreateBuiltinVideoEncoderFactory(),
            ::webrtc::CreateBuiltinVideoDecoderFactory(),
            nullptr /* audio_mixer */, nullptr /* audio_processing */);
}

void Switchboard::rtcConnectionDropped(std::string participant) {
    rtc::CritScope cs(&mCleanupCS);
    mDroppedConnections.push_back(participant);
}

void Switchboard::rtcConnectionClosed(std::string participant) {
    rtc::CritScope cs(&mCleanupClosedCS);
    mClosedConnections.push_back(participant);
}

void Switchboard::finalizeConnections() {
    {
        rtc::CritScope cs(&mCleanupCS);
        for (auto participant : mDroppedConnections) {
            AutoWriteLock lock(mMapLock);
            if (mId.find(participant) != mId.end()) {
                auto queue = mId[participant];
                mLocks.erase(queue.get());
                mId.erase(participant);
            }
            mConnections[participant]->Close();
        }
        mDroppedConnections.clear();
    }

    {
        rtc::CritScope cs(&mCleanupClosedCS);
        for (auto participant : mClosedConnections) {
            mIdentityMap.erase(participant);
            mConnections.erase(participant);
        }
        mClosedConnections.clear();
    }
}

void Switchboard::send(std::string to, json message) {
    RTC_LOG(INFO) << "send to: " << to << " msg: " << message.dump();
    if (mId.find(to) != mId.end()) {
        auto queue = mId[to];
        {
            AutoLock pushLock(*mLocks[queue.get()]);
            if (queue->tryPushLocked(message.dump()) != BufferQueueResult::Ok) {
                LOG(ERROR) << "Unable to push message " << message.dump()
                           << "dropping it";
            }
        }
    } else {
        RTC_LOG(WARNING) << "Sending to an unknown participant!";
    }
}

bool Switchboard::connect(std::string identity) {
    RTC_LOG(INFO) << "Connecting: " << identity;
    mMapLock.lockRead();
    if (mId.find(identity) == mId.end()) {
        mMapLock.unlockRead();
        AutoWriteLock upgrade(mMapLock);
        std::shared_ptr<Lock> bufferLock(new Lock());
        std::shared_ptr<MessageQueue> queue(
                new MessageQueue(kMaxMessageQueueLen, *(bufferLock.get())));
        mId[identity] = queue;
        mLocks[queue.get()] = bufferLock;
    } else {
        mMapLock.unlockRead();
    }

    rtc::scoped_refptr<Participant> stream(
            new rtc::RefCountedObject<Participant>(this, identity,
                                                   mConnectionFactory.get()));
    if (!stream->Initialize()) {
        return false;
    }
    mConnections[identity] = stream;

    // Note: Dynamically adding new tracks will likely require stream
    // re-negotiation.
    stream->AddVideoTrack("grpcVideo");
    stream->AddAudioTrack("grpcAudio");
    stream->CreateOffer();

    // Inform the client we are going to start, this is the place
    // where we should send the turn config to the client.
    // Turn is really only needed when your server is
    // locked down pretty well. (Google gce environment for example)
    json turnConfig = "{}"_json;
    if (mTurnConfig.size() > 0) {
        System::ProcessExitCode exitCode;
        Optional<std::string> turn = System::get()->runCommandWithResult(
                mTurnConfig, kMaxTurnConfigTime, &exitCode);
        if (exitCode == 0 && turn && json::accept(*turn)) {
            json config = json::parse(*turn, nullptr, false);
            if (config.count("iceServers")) {
                turnConfig = config;
            } else {
                RTC_LOG(LS_ERROR) << "Unusable turn config: " << turn;
            }
        }
    }

    // The format is json: {"start" : RTCPeerConnection config}
    // (i.e. result from
    // https://networktraversal.googleapis.com/v1alpha/iceconfig?key=....)
    json start;
    start["start"] = turnConfig;
    send(identity, start);

    return true;
}

void Switchboard::disconnect(std::string identity) {
    {
        AutoReadLock lock(mMapLock);
        if (mId.find(identity) == mId.end()) {
            LOG(ERROR) << "Trying to remove unknown queue, ignoring";
            return;
        }
    }
    LOG(INFO) << "disconnect: " << identity;
    AutoWriteLock upgrade(mMapLock);
    // Unlikely but someone could have yanked identity out of the map
    if (mId.find(identity) == mId.end()) {
        LOG(ERROR) << "Trying to remove unknown queue, ignoring";
        return;
    }

    auto queue = mId[identity];
    mId.erase(identity);
    mLocks.erase(queue.get());
    mIdentityMap.erase(identity);
    mConnections.erase(identity);
}

bool Switchboard::acceptJsepMessage(std::string identity, std::string message) {
    AutoReadLock lock(mMapLock);
    if (mId.count(identity) == 0 || mConnections.count(identity) == 0) {
        LOG(ERROR) << "Trying to send to unknown identity.";
        return false;
    }
    if (!json::accept(message)) {
        return false;
    }
    mConnections[identity]->IncomingMessage(json::parse(message));
    return true;
}

bool Switchboard::nextMessage(std::string identity,
                              std::string* nextMessage,
                              System::Duration blockAtMostMs) {
    std::shared_ptr<MessageQueue> queue;
    std::shared_ptr<Lock> lock;
    {
        AutoReadLock readerLock(mMapLock);
        if (mId.find(identity) == mId.end()) {
            LOG(ERROR) << "Unknown identity: " << identity;
            *nextMessage =
                    std::move(std::string("{ \"bye\" : \"disconnected\" }"));
            return false;
        }
        queue = mId[identity];
        lock = mLocks[queue.get()];
        lock->lock();
    }

    System::Duration blockUntil =
            System::get()->getUnixTimeUs() + blockAtMostMs * 1000;
    bool status = queue->popLockedBefore(nextMessage, blockUntil) ==
                  BufferQueueResult::Ok;
    lock->unlock();
    return status;
}

}  // namespace webrtc
}  // namespace emulator
