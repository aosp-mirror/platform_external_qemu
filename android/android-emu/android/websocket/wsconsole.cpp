#include "android/base/log.h"
#include "android/websocket/json.hpp"
#include "android/websocket/wsconsole.h"

#include <string.h>
#include <iostream>

using namespace std;
using namespace android::base;
using json = nlohmann::json;

extern "C" {
void initialize_webrtc(void) {
    WSConsole* es = new WSConsole(8080);
    es->start();
}
}

namespace android {

namespace base {
void to_json(json& j, const pubsub_msg& p) {
    // We do not support serialization right now..
    // j = json{{"from", p.from}, {"topic", p.topic}, {"msg", p.msg}, {"type":
    // (int) p.type }};
}

void from_json(const json& j, pubsub_msg& p) {
    if (j.find("topic") != j.end())
        p.topic = j.at("topic").get<std::string>();
    if (j.find("msg") != j.end())
        p.msg = j.at("msg").get<std::string>();
    if (j.find("type") != j.end())
        p.type = j.at("type").get<std::string>();
}

}  // namespace base
}  // namespace android

intptr_t WSConsole::main() {
    this->run();
    return 0;
}

WSConsole::WSConsole(int port)
    : WebSocketServer(port), Thread(), mPubsubQueue(&mPubsub), mEmulator(&mPubsub) {}

WSConsole::~WSConsole() {}

void WSConsole::onConnect(int id) {
    AutoLock lock(mLock);
    mClientMap[id] = std::unique_ptr<WebClient>(new WebClient(id, this));
}

void WSConsole::onMessage(int id, const string& data) {
    AutoLock lock(mLock);
    if (mClientMap.find(id) == mClientMap.end()) {
        LOG(ERROR) << "refusing to handle message from client " << id << " that has left";
        return;
    }

    Receiver* r = mClientMap[id].get();
    pubsub_msg msg = json::parse(data);
}

void WSConsole::onDisconnect(int id) {
    AutoLock lock(mLock);
    mPubsub.disconnect(mClientMap[id].get());
    mClientMap.erase(id);
}

void WSConsole::onError(int id, const string& message) {
    fprintf(stderr, "%s: %s\n", __func__, message.c_str());
}

WebClient::WebClient(int id, WSConsole* console) : mId(id), mConsole(console) {}

void WebClient::send(std::string msg) {
    printf("Sending %s\n", msg.c_str());
    mConsole->send(mId, msg);
}
