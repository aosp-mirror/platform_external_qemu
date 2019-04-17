#pragma once

#include <string>
#include <emulator/net/JsonProtocol.h>
#include <string>
#include "Participant.h"
namespace emulator {
namespace webrtc {

using net::SocketTransport;
using net::JsonReceiver;
using net::JsonProtocol;
using net::State;

class Participant;

// A switchboard manages a set of web rtc participants:
//
// 1. It creates new participants when a user starts a session
// 2. It removes participants when a user disconnects
// 3. It routes the webrtc json signals to the proper participant.
class Switchboard : public JsonReceiver {
 public:
  Switchboard(std::string handle);
  ~Switchboard();
  void received(SocketTransport* from, const json object) override;
  void stateConnectionChange(SocketTransport* connection, State current) override;
  void send(std::string to, json msg);
  void connect(std::string server, int port);

 private:
  std::map<std::string, rtc::scoped_refptr<Participant> > mConnections;
  std::map<std::string, std::string> mIdentityMap;
  std::string handle_;
  JsonProtocol mProtocol;
  SocketTransport mTransport;
};
}
}
