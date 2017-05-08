#include "android/websocket/wsconsole.h"

using namespace std;
extern "C" {
  void android_run_server() {
    WSConsole es( 8080 );
    es.run( );
  }
}

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
    memcpy(me->buff, data.c_str(), std::min(data.size(), 4096ul));
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
