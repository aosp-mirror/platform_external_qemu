#include "android/websocket/wsconsole.h"
#include "android/websocket/webrtc.h"

#include <string.h>
#include <iostream> 

class IceThread : public android::base::Thread {

  virtual intptr_t main() override {
    sleep(5);
    webrtc_start_ice_start();
    return 0;
  }
};

extern "C" {
  void initialize_webrtc(void) {
    WSConsole* es = new WSConsole(8080);
    es->start();
    IceThread* ice = new IceThread();
    ice->start();
  }
}


using namespace std;
WSConsole::WSConsole( int port ) : WebSocketServer( port ), Thread() {
}

WSConsole::~WSConsole( ) {
}


void WSConsole::onConnect( int socketID ) {
    fprintf(stderr, "%s\n", __func__);
    me = control_client_create_empty();
    me->opaque = this;
    me->write = WSConsole_send;
    me->sock = socketID;
}

void WSConsole::onMessage( int socketID, const string& data ) {
    fprintf(stderr, "%s: %s\n", __func__, data.c_str());
    memset(me->buff, 0, 4096);
    memcpy(me->buff, data.c_str(), std::min(4096, (int) data.size()));
    me->buff_len = data.size();
    control_client_do_command(me);
}

void WSConsole_send(ControlClient client, const char* msg, int cMsg) {
  WSConsole *srv = reinterpret_cast<WSConsole*>(client->opaque);
  srv->send(client->sock, std::string(msg, cMsg));
}

void WSConsole::onDisconnect( int socketID ) {
    fprintf(stderr, "%s\n", __func__);
}

void WSConsole::onError( int socketID, const string& message ) {
    fprintf(stderr, "%s: %s\n", __func__, message.c_str());
}
