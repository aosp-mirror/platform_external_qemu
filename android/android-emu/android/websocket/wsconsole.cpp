#include "android/base/Log.h"
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
/** Simple (json) message format for our pubsub protocol */
struct pubsub_msg {
    std::string topic;
    std::string msg;
    std::string type;
    std::string from;
};

void to_json(json& j, const pubsub_msg& p) {
    j["from"] = p.from;
    j["topic"] = p.topic;
    j["type"] = p.type;
    j["msg"] = p.msg;
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
    : WebSocketServer(port), Thread(), mEmulator(&mPubsub) {}

WSConsole::~WSConsole() {}

void WSConsole::onConnect(int id) {
    AutoLock lock(mLock);
    WebClient* wc = new WebClient(id, this);
    mClientMap[id] = std::unique_ptr<WebClient>(wc);
    mPubsub.connect(wc);
}

void WSConsole::onMessage(int id, const string& data) {
    AutoLock lock(mLock);
    if (mClientMap.find(id) == mClientMap.end()) {
        LOG(ERROR) << "refusing to handle message from client " << id << " that has left";
        return;
    }

    Receiver* r = mClientMap[id].get();
    pubsub_msg msg = json::parse(data);
    msg.from = std::to_string(id);
    if (msg.type == "subscribe") {
      mPubsub.subscribe(r, msg.topic);
    } else if (msg.type == "unsubscribe") {
      mPubsub.unsubscribe(r, msg.topic);
    } else if (msg.type == "publish") {
      mPubsub.publish(r, msg.topic, msg.msg);
    } else {
      LOG(ERROR) << "unknown command " << msg.type;
    }
}

void WSConsole::onDisconnect(int id) {
    AutoLock lock(mLock);
    mPubsub.disconnect(mClientMap[id].get());
    mClientMap.erase(id);
}

void WSConsole::onError(int id, const string& message) {
  LOG(ERROR) << "Error from client " << id << ", msg: " << message;
}

WebClient::WebClient(int id, WSConsole* console) : mId(id), mConsole(console) {}

void WebClient::send(Receiver* from, Topic t, std::string msg) {
    pubsub_msg psm = { t, msg, "publish", from->id() };
    json jmsg = psm;
    std::string message = jmsg.dump(2);
    LOG(INFO) << "To: " << id() << ", msg: " << message;
    mConsole->send(mId, message);
}
