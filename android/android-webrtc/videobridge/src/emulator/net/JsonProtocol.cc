#include "JsonProtocol.h"
#include <rtc_base/logging.h>
#include "SocketTransport.h"


namespace emulator {
namespace net {
bool JsonProtocol::write(SocketTransport* to, const json object) {
  std::string msg = (object.dump() + '\0');
  return to->send(msg);
}

void JsonProtocol::received(SocketTransport* from, const std::string object) {
  for (unsigned int i = 0; i < object.size(); i++) {
    char c = object.c_str()[i];
    if (c == '\0') {
      if (json::accept(mBuffer)) {
        json jmessage = json::parse(mBuffer, nullptr, false);
        mReceiver->received(from, std::move(jmessage));
      }
      mBuffer.clear();
    } else {
      mBuffer += c;
    }
  }
}

void JsonProtocol::stateConnectionChange(SocketTransport* connection,
                                         State current) {
  mReceiver->stateConnectionChange(connection, current);
}
}
}
