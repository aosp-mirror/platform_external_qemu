#include "emulator/net/waterfall/WaterfallProtocol.h"
#include "emulator/net/testing/TestAsynSocketAdapter.h"
#include "emulator/net/SocketTransport.h"

#include <gtest/gtest.h>

#include <vector>

namespace emulator {
namespace net {

// Waterfall protocol translates to and from std::string..
using SimpleWaterfallReader = SimpleProtocolReader<std::string>;

TEST(waterfallProtocol, identity) {
    // Identity serializer.. a--> .. --> a
    SimpleWaterfallReader reader;
    TestAsyncSocketAdapter* socket = new TestAsyncSocketAdapter({});

    WaterfallProtocol waterfallProtocol(&reader);
    SocketTransport transport(&waterfallProtocol, socket);
    waterfallProtocol.write(&transport, "Hello world");
    socket->loopback();

    EXPECT_EQ(reader.mReceived.size(), 1);
    EXPECT_STREQ(reader.mReceived[0].c_str(), "Hello world");
}

}  // namespace net
}  // namespace emulator