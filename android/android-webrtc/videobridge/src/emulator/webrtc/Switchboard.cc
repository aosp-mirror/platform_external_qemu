#include "Switchboard.h"
#include "rtc_base/logging.h"

namespace emulator {
namespace webrtc {

void Switchboard::stateConnectionChange(SocketTransport* connection,
                                        State current) {
    if (current == State::CONNECTED) {
        // Connect to pubsub
        mProtocol.write(connection, {{"type", "publish"},
                                     {"topic", "connected"},
                                     {"msg", "switchboard"}});
        // Subscribe to disconnect events.
        mProtocol.write(connection,
                        {
                            {"type", "subscribe"}, {"topic", "disconnected"},
                        });
    }

    // We got disconnected, clear out all participants.
    if (current == State::NOT_CONNECTED) {
        mConnections.clear();
        mIdentityMap.clear();
    }
}

void Switchboard::connect(std::string server, int port) {
    mTransport.connect(server, port);
}

Switchboard::~Switchboard() {}
Switchboard::Switchboard(std::string handle)
      : handle_(handle), mProtocol(this), mTransport(&mProtocol) {}

void Switchboard::received(SocketTransport* transport, const json object) {
    RTC_LOG(INFO) << "Received: " << object;

    // We expect:
    // { 'msg' : some_json_object,
    //   'from': some_sender_string,
    // }
    if (!object.count("msg") || !object.count("from") ||
        !json::accept(object["msg"].get<std::string>())) {
        RTC_LOG(LERROR) << "Message not according spec, ignoring!";
        return;
    }

    std::string from = object["from"];
    json msg = json::parse(object["msg"].get<std::string>(), nullptr, false);

    // Start a new participant.
    if (msg.count("start")) {
        mIdentityMap[from] = msg["start"];
        rtc::scoped_refptr<Participant> stream(
            new rtc::RefCountedObject<Participant>(this, from, handle_));
        if (stream->Initialize()) {
            mConnections[from] = stream;
        }
        return;
    }

    // Client disconnected
    if (msg.is_string() && msg.get<std::string>() == "disconnected" &&
        mConnections.count(from)) {
        mIdentityMap.erase(from);
        mConnections.erase(from);
    }

    // Route webrtc signaling packet
    if (mConnections.count(from)) {
        mConnections[from]->IncomingMessage(msg);
    }
}

void Switchboard::send(std::string to, json message) {
    if (!mIdentityMap.count(to)) {
        return;
    }

    json msg;
    msg["type"] = "publish";
    msg["msg"] = message.dump();
    msg["topic"] = mIdentityMap[to];

    RTC_LOG(INFO) << "Sending " << msg;
    mProtocol.write(&mTransport, msg);
}
}
}
