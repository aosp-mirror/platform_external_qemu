#include "android/websocket/wsconsole.h"
#include "android/websocket/webrtc.h"
#include "android/websocket/json.hpp"

#include <string.h>
#include <iostream> 
using namespace std;
using json = nlohmann::json;

extern "C" {
  void initialize_webrtc(void) {
    WSConsole* es = new WSConsole(8080);
    es->start();
  }
}


enum Command  { Connect = 0, Disconnect, Sub, Unsub, Publish};

struct pubsub {
   std::string from;
   std::string topic;
   std::string msg;
   int type;
};

namespace {
    void to_json(json& j, const pubsub& p) {
      // We do not support serialization right now..
       // j = json{{"from", p.from}, {"topic", p.topic}, {"msg", p.msg}, {"type": (int) p.type }};
    }

    void from_json(const json& j, pubsub& p) {
        p.from= j.at("from").get<std::string>();
        p.topic = j.at("topic").get<std::string>();
        p.msg = j.at("msg").get<std::string>();
        p.type = j.at("type").get<int>();
    }
}

WSConsole::WSConsole( int port ) : WebSocketServer( port ), Thread() {
}

WSConsole::~WSConsole( ) { }


void WSConsole::onConnect( int socketID ) {
    fprintf(stderr, "%s\n", __func__);
}

void WSConsole::onMessage( int socketID, const string& data ) {
    fprintf(stderr, "%s: %s\n", __func__, data.c_str());
    json msg = json::parse(data);
}


void WSConsole::onDisconnect( int socketID ) {
    fprintf(stderr, "%s\n", __func__);
}

void WSConsole::onError( int socketID, const string& message ) {
    fprintf(stderr, "%s: %s\n", __func__, message.c_str());
}
