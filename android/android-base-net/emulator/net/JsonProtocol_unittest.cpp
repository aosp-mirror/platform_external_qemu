#include "emulator/net/JsonProtocol.h"
#include "emulator/net/testing/TestAsynSocketAdapter.h"
#include <gtest/gtest.h>

#include <vector>

namespace emulator {
namespace net {

using SimpleJsonReader = SimpleProtocolReader<json>;

json hello_world = {{"hello", "world"}};

TEST(JsonProtocolTest, receiveMessage) {
    SimpleJsonReader reader;
    JsonProtocol jsonProtocol(&reader);
    std::string msg(hello_world.dump());
    msg.append(1, '\000');
    jsonProtocol.received(nullptr, msg);
    EXPECT_EQ(reader.mReceived.size(), 1);
    EXPECT_STREQ(reader.mReceived[0].dump().c_str(), hello_world.dump().c_str());
}

TEST(JsonProtocolTest, receiveMessageFromParts) {
    SimpleJsonReader reader;
    JsonProtocol jsonProtocol(&reader);
    std::string msg("{ \"hello\" : \"world\"}");
    msg.append(1, '\000');
    for (int i = 0; i < msg.size(); i++) {
        std::string partial(1, msg.at(i));
        jsonProtocol.received(nullptr, partial);
    }
    EXPECT_EQ(reader.mReceived.size(), 1);
    EXPECT_STREQ(reader.mReceived[0].dump().c_str(), hello_world.dump().c_str());
}

TEST(JsonProtocolTest, identity) {
    // Identity serializer.. a--> .. --> a
    SimpleJsonReader reader;
    TestAsyncSocketAdapter* socket = new TestAsyncSocketAdapter({});

    JsonProtocol jsonProtocol(&reader);
    SocketTransport transport(&jsonProtocol, socket);
    jsonProtocol.write(&transport, hello_world);
    socket->loopback();

    EXPECT_EQ(reader.mReceived.size(), 1);
    EXPECT_STREQ(reader.mReceived[0].dump().c_str(), hello_world.dump().c_str());
}


}  // namespace net
}  // namespace emulator