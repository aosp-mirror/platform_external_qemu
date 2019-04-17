#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include "SocketTransport.h"

using json = nlohmann::json;
namespace emulator {
namespace net {

using JsonReceiver = ProtocolReader<json>;
using JsonWriter = ProtocolWriter<json>;


// A simple wire protocol that sends and receive json messages
// that are seperated by '\0000' (null).
//
// When a new json message has been parsed it will be passed on
// to the receiver. Socket state changes will be forwarded.
class JsonProtocol : public MessageReceiver, public JsonWriter {
 public:
  JsonProtocol(JsonReceiver* receiver) : mReceiver(receiver) {}

  // Wites a json blob to the socket.
  bool write(SocketTransport* to, const json object) override;

  // Called when the socket has bytes
  void received(SocketTransport* from, const std::string object) override;

  // Called when the socket changes state.
  void stateConnectionChange(SocketTransport* connection,
                             State current) override;

 private:
  ProtocolReader<json>* mReceiver;
  std::string mBuffer;
};
}
}
