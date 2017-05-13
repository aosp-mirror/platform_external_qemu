// Copyright (C) 2017 The Android Open Source Project
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

#include <set>
#include <unordered_map>
#include <utility>
#include <vector>
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/threads/Thread.h"

namespace android {
namespace base {

class Receiver {
public:
    virtual void send(std::string msg);
};

class Pubsub {
    using Topic = std::string;

public:
    void connect(Receiver* r);
    void disconnect(Receiver* r);
    void publish(Topic t, std::string msg);
    void subscribe(Receiver* r, Topic t);
    void unsubscribe(Receiver* r, Topic t);

private:
    std::unordered_map<Topic, std::set<Receiver*>> mTopics;
    std::unordered_map<Receiver*, std::set<Topic>> mSubscribed;
    std::set<Receiver*> mConnected;
    Lock mLock;
};

template <typename T, typename DISPATCH, size_t CAPACITY>
class PubsubQueue : public android::base::Thread {
public:
    DISALLOW_COPY_ASSIGN_AND_MOVE(PubsubQueue);
    PubsubQueue(Pubsub* pubsub) : mPubsub(pubsub) { this->start(); }

    virtual intptr_t main() override {
        std::pair<Receiver*, T>* msg;
        while (mChannel.receive(msg)) {
            DISPATCH::dispatch(mPubsub, std::get<0>(*msg), std::get<1>(*msg));
        }
        LOG(INFO) << "Finished Processing";
    }

    void enqueue(Receiver* r, const T& msg) {
        LOG(INFO) << "enqueue:  receiver: " << r << ", msg: " << msg;
        mChannel.send(std::make_pair(r, msg));
    }

    void stop() { mChannel.stop(); }

private:
    MessageChannel<std::pair<Receiver*, T>, CAPACITY> mChannel;
    Pubsub* mPubsub;
    bool mStop;
};

}  // namespace base
}  // namespace android
