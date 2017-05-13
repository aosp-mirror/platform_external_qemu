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
#include <unordered_map>

#include "android/base/synchronization/Lock.h"
#include "android/base/threads/Thread.h"
#include "android/websocket/console-adapter.h"
#include "android/websocket/pubsub.h"
#include "android/websocket/websocketserver.h"

namespace android {
namespace base {
class WebClient;

/** Simple (json) message format for our pubsub protocol */
struct pubsub_msg {
    std::string topic;
    std::string msg;
    std::string type;
};

/** Dispatcher that knows what to do with ta pubsub_msg */
struct Dispatch_pubsub_msg {
  static void dispatch(Pubsub* pubsub, Receiver* r, pubsub_msg msg) {
    if (msg.type == "connect") {
      pubsub->connect(r);
    } else if (msg.type == "disconnect") {
      pubsub->disconnect(r);
    } else if (msg.type == "subscribe") {
      pubsub->subscribe(r, msg.topic);
    } else if (msg.type == "unsubscribe") {
      pubsub->unsubscribe(r, msg.topic);
    } else if (msg.type == "publish") {
      pubsub->publish(msg.topic, msg.msg);
    } else {
      LOG(ERROR) << "unknown command " << msg.type;
    }
  }
};

class WSConsole : public WebSocketServer, public android::base::Thread {
    DISALLOW_COPY_ASSIGN_AND_MOVE(WSConsole);

public:
    WSConsole(int port);
    ~WSConsole();

    virtual void onConnect(int socketID) override;
    virtual void onMessage(int socketID, const string& data) override;
    virtual void onDisconnect(int socketID) override;
    virtual void onError(int socketID, const string& message) override;
    virtual intptr_t main() override;

private:
    Pubsub mPubsub;
    PubsubQueue<pubsub_msg, Dispatch_pubsub_msg, 3000> mPubsubQueue;
    ConsoleAdapter mEmulator;
    std::unordered_map<int, std::unique_ptr<WebClient>> mClientMap;
    Lock mLock;
};

class WebClient : public Receiver {
    DISALLOW_COPY_ASSIGN_AND_MOVE(WebClient);

public:
    WebClient(int id, WSConsole* console);
    virtual void send(std::string msg) override;

private:
    int mId;
    WSConsole* mConsole;
};

}  // namespace base
}  // namespace android
