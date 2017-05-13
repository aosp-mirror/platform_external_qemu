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
#include <functional>
#include <vector>
#include "android/base/Log.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/threads/Thread.h"

namespace android {
namespace base {

using Topic = std::string;

class Receiver {
public:
    virtual void send(Receiver* from, Topic t, std::string msg) = 0;
    virtual std::string id() = 0;
};


/**
 * A queue of pubsub messages of type T that are interpreted by the DISPATCH
 * policy.
 */
template <size_t CAPACITY>
class PubsubQueue : public android::base::Thread {
public:
    DISALLOW_COPY_ASSIGN_AND_MOVE(PubsubQueue);
    PubsubQueue() { this->start(); }

    virtual intptr_t main() override {
        std::function<void()> f;
        while (mChannel.receive(&f)) {
          f();
        }
        return 1;
    }

    void submit(std::function<void()> f) {
        mChannel.send(f);
    }

    void stop() { mChannel.stop(); }

private:
    MessageChannel<std::function<void()>, CAPACITY> mChannel;
};

class Pubsub {
    DISALLOW_COPY_ASSIGN_AND_MOVE(Pubsub);

public:
    Pubsub() {}
    void connect(Receiver* r);
    void disconnect(Receiver* r);
    void publish(Receiver* from, Topic t, std::string msg);
    void subscribe(Receiver* r, Topic t);
    void unsubscribe(Receiver* r, Topic t);
    void stop();

private:
    std::unordered_map<Topic, std::set<Receiver*>> mTopics;
    std::unordered_map<Receiver*, std::set<Topic>> mSubscribed;
    std::set<Receiver*> mConnected;
    PubsubQueue<3000> mWorkerQueue;
};


}  // namespace base
}  // namespace android
