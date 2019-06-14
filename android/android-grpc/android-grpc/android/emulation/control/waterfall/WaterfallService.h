#include <sys/socket.h>

#include <string>
#include <vector>

#include "android/base/Log.h"
#include "android/base/async/AsyncWriter.h"
#include "android/base/async/Looper.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/containers/BufferQueue.h"
#include "android/base/sockets/AsyncSocket.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/threads/FunctorThread.h"
#include "android/utils/path.h"
#include "android/utils/sockets.h"
#include "emulator/net/AsyncSocketAdapter.h"
#include "emulator/net/SocketTransport.h"
#include "emulator/net/waterfall/WaterfallProtocol.h"
#include "grpcpp/create_channel_posix.h"

namespace android {
namespace emulation {
namespace control {

using MessageQueue = android::base::BufferQueue<std::string>;
using android::base::AsyncSocketFd;
using android::base::Lock;
using android::base::Looper;
using emulator::net::MessageReceiver;
using emulator::net::SocketTransport;
using emulator::net::State;
using emulator::net::WaterfallProtocol;
using emulator::net::WaterfallReceiver;

class WaterfallDecoder : public WaterfallReceiver {
public:
    WaterfallDecoder(Looper* looper,
                     AsyncSocketFd* incomingFd,
                     AsyncSocketFd* outgoingFd);
    ~WaterfallDecoder() = default;
    void close();
    void received(SocketTransport* from, std::string msg) override;
    void stateConnectionChange(SocketTransport* connection,
                               State current) override;

private:
    AsyncSocketFd* mOutgoingFd;
    WaterfallProtocol mWaterfall;
    SocketTransport mIncoming;
};  // namespace net

class WaterfallEncoder : public MessageReceiver {
public:
    WaterfallEncoder(Looper* looper,
                     AsyncSocketFd* incomingFd,
                     AsyncSocketFd* outgoingFd);
    ~WaterfallEncoder() = default;
    void close();
    void received(SocketTransport* from, std::string msg) override;
    void stateConnectionChange(SocketTransport* connection,
                               State current) override;

private:
    WaterfallProtocol mWaterfall;
    SocketTransport mIncoming;
    SocketTransport mOutgoing;
};

class WaterfallBridge {
public:
    WaterfallBridge(Looper* looper, int serverFd, int fd);
    ~WaterfallBridge() {}
    void close();
    std::shared_ptr<grpc::Channel> getGrpcChannel();

private:
    int handshake();
    std::shared_ptr<grpc::Channel> openChannel();

    Looper* mLooper;
    int mSocket;
    int mWaterfallFd;
    std::unique_ptr<AsyncSocketFd> mWaterfallSocket;
    std::unique_ptr<AsyncSocketFd> mGrpcSocket;
    std::unique_ptr<WaterfallDecoder> mDecoder;
    std::unique_ptr<WaterfallEncoder> mEncoder;
    std::shared_ptr<grpc::Channel> mChannel;
};

class WaterfallService {
public:
    WaterfallService();
    WaterfallBridge* getBridge();

private:
    void incoming();

    const char* mWaterfallPath = "sockets";
    const char* mWaterfallName = "sockets/h2o";
    int mSocket;
    int mWaterfallFd;

    Looper* mLooper;
    base::FunctorThread mAcceptor;
};
}  // namespace control
}  // namespace emulation
}  // namespace android
