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
#include "emulator/webrtc/Switchboard.h"

#include <api/scoped_refptr.h>  // for scoped_refptr
#include <api/units/time_delta.h>
#include <rtc_base/logging.h>             // for Val, RTC_LOG
#include <rtc_base/ref_counted_object.h>  // for RefCountedObject
#include <map>                            // for map, operator==
#include <memory>                         // for shared_ptr, make_sh...
#include <ostream>                        // for operator<<, ostream
#include <string>                         // for string, hash, opera...
#include <type_traits>                    // for remove_extent_t
#include <unordered_map>                  // for unordered_map, unor...
#include <utility>                        // for move
#include <vector>                         // for vector


#include "aemu/base/Log.h"                     // for LogStreamVoidify, LOG
#include "aemu/base/async/AsyncSocket.h"       // for AsyncSocket
#include "aemu/base/containers/BufferQueue.h"  // for BufferQueueResult
#include "aemu/base/synchronization/Lock.h"    // for Lock, AutoReadLock
#include "android/base/system/System.h"        // for System, System::Dur...
#include "android/console.h"                   // for getConsoleAgents
#include "emulator/webrtc/Participant.h"       // for Participant
#include "emulator/webrtc/RtcConnection.h"     // for json, RtcConnection
#include "nlohmann/json.hpp"                   // for basic_json<>::value...

using namespace android::base;

namespace emulator {
namespace webrtc {

std::string Switchboard::BRIDGE_RECEIVER = "WebrtcVideoBridge";

Switchboard::~Switchboard() {}

Switchboard::Switchboard(TurnConfig turnConfig,
                         const std::string& audioDumpFile)
    : RtcConnection(), mTurnConfig(turnConfig), mAudioDumpFile(audioDumpFile) {}

void Switchboard::rtcConnectionClosed(const std::string participant) {
    const std::lock_guard<std::mutex> lock(mCleanupMutex);

    if (mId.find(participant) != mId.end()) {
        LOG(DEBUG) << "Finalizing " << participant;
        auto queue = mId[participant];
        mLocks.erase(queue.get());
        mId.erase(participant);
    }

    signalingThread()->PostDelayedTask(
            [this, participant] {
                const std::lock_guard<std::mutex> lock(mCleanupMutex);
                LOG(DEBUG) << "disconnect: " << participant
                          << ", available: " << mConnections.count(participant);
                auto it = mConnections.find(participant);
                if (it != mConnections.end()) {
                    it->second->Close();
                    mConnections.erase(participant);
                }
            },
            ::webrtc::TimeDelta::Seconds(0));
}

void Switchboard::send(std::string to, json message) {
    LOG(DEBUG) << "send to: " << to << " msg: " << message.dump();
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
        LOG(WARNING) << "Sending to an unknown participant!";
    }
}

bool Switchboard::connect(std::string identity) {
    // Inform the client we are going to start, this is the place
    // where we should send the turn config to the client.
    // Turn is really only needed when your server is
    // locked down pretty well. (Google gce environment for example)
    return connect(std::move(identity), mTurnConfig.getConfig());
}

bool Switchboard::connect(std::string identity, std::string turnConfig) {
    auto j = json::parse(turnConfig, nullptr, false);
    if (j.is_discarded()) {
        LOG(ERROR) << "Failed to parse turnConfig to json. TurnConfig: "
                   << turnConfig;
        return false;
    }
    return connect(std::move(identity), j["ice_servers"]);
}

bool Switchboard::connect(std::string identity, json turnConfig) {
    LOG(DEBUG) << "Connecting: " << identity;
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

    auto participant =
            std::make_shared<Participant>(this, identity, turnConfig, nullptr);
    if (!participant->Initialize()) {
        LOG(ERROR) << "Failed to initialize the participant with id: "
                   << identity << " and turnConfig" << turnConfig.dump();
        return false;
    }
    mConnections[identity] = participant;

    // The format is json: {"start" : RTCPeerConnection config}
    // (i.e. result from
    // https://networktraversal.googleapis.com/v1alpha/iceconfig?key=....)
    send(identity, json{{"start", turnConfig}});

    // Note: Dynamically adding new tracks will likely require participant
    // re-negotiation.
    const auto* agent = getConsoleAgents();
    if (agent && agent->multi_display->isMultiDisplayEnabled()) {
        int32_t startId = -1;
        uint32_t id;
        while (agent->multi_display->getNextMultiDisplay(
                startId, &id, nullptr, nullptr, nullptr, nullptr, nullptr,
                nullptr, nullptr)) {
            LOG(DEBUG) << "Add video track with displayId: " << id;
            participant->AddVideoTrack(id);
            startId = id;
        }
    } else {
        LOG(DEBUG) << "Add default video track 0";
        participant->AddVideoTrack(0);
    }
    participant->AddAudioTrack(mAudioDumpFile);
    participant->CreateOffer();

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
    LOG(DEBUG) << "disconnect: " << identity;
    AutoWriteLock upgrade(mMapLock);
    // Unlikely but someone could have yanked identity out of the map
    if (mId.find(identity) == mId.end()) {
        LOG(ERROR) << "Trying to remove unknown queue, ignoring";
        return;
    }

    auto queue = mId[identity];
    mId.erase(identity);
    mLocks.erase(queue.get());

    const std::lock_guard<std::mutex> lock(mCleanupMutex);
    mConnections[identity]->Close();
}

bool Switchboard::acceptJsepMessage(std::string identity, std::string message) {
    AutoReadLock lock(mMapLock);
    if (mId.count(identity) == 0 || mConnections.count(identity) == 0) {
        LOG(ERROR) << "Trying to send to unknown identity.";
        return false;
    }
    auto parsedMessage = json::parse(message, nullptr, false);
    if (parsedMessage.is_discarded()) {
        return false;
    }
    mConnections[identity]->IncomingMessage(parsedMessage);
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
