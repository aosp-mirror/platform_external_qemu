#include "android/emulation/control/waterfall/WaterfallService.h"
namespace android {
namespace emulation {
namespace control {

WaterfallDecoder::WaterfallDecoder(Looper* looper,
                                   AsyncSocketFd* incomingFd,
                                   AsyncSocketFd* outgoingFd)
    : mWaterfall(this),
      mOutgoingFd(outgoingFd),
      mIncoming(&mWaterfall, incomingFd, SocketTransport::TransportMode::Read) {
}

void WaterfallDecoder::close() {
    mIncoming.close();
    mOutgoingFd->close();
}

void WaterfallDecoder::received(SocketTransport* from, std::string msg) {
    mOutgoingFd->send(msg.data(), msg.size());
}

void WaterfallDecoder::stateConnectionChange(SocketTransport* connection,
                                             State current) {
    if (current == State::DISCONNECTED) {
        mOutgoingFd->close();
    }
}

WaterfallEncoder::WaterfallEncoder(Looper* looper,
                                   AsyncSocketFd* incomingFd,
                                   AsyncSocketFd* outgoingFd)
    : mWaterfall(nullptr),
      mOutgoing(nullptr, outgoingFd, SocketTransport::TransportMode::Write),
      mIncoming(this, incomingFd, SocketTransport::TransportMode::Read) {}

void WaterfallEncoder::received(SocketTransport* from, std::string msg) {
    mWaterfall.write(&mOutgoing, msg);
}

void WaterfallEncoder::close() {
    mOutgoing.close();
    mIncoming.close();
}

void WaterfallEncoder::stateConnectionChange(SocketTransport* connection,
                                             State current) {
    if (current == State::DISCONNECTED) {
        mOutgoing.close();
    }
}

WaterfallService::WaterfallService() : mAcceptor([this] { this->incoming(); }) {
    SockAddress addr = {};
    path_mkdir_if_needed(mWaterfallPath, 0755);
    unlink(mWaterfallName);
    addr.u._unix.path = mWaterfallName;
    addr.family = SOCKET_UNIX;

    mSocket = socket_create(SOCKET_UNIX, SOCKET_STREAM);
    LOG(INFO) << "Socket:" << mSocket;
    int res = socket_bind(mSocket, &addr);
    LOG(INFO) << "bind:" << res;
    res = socket_listen(mSocket, 1);
    LOG(INFO) << "listen:" << res;

    mLooper = android::base::ThreadLooper::get();
    mAcceptor.start();
};

void WaterfallService::incoming() {
    while (mWaterfallFd <= 0) {
        int fd = accept(mSocket, 0, 0);
        if (fd < 0) {
            LOG(ERROR) << "Failed to accept socket:" << errno << ", " << errno_str;
        } else {
            mWaterfallFd = fd;
        }
    }
    // Set socket timeouts to be pretty brief, so we can handshake
    // synchronously.

    // TODO Make a global const
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if (setsockopt(mWaterfallFd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout,
                   sizeof(timeout)) < 0)
        LOG(ERROR) << "setsockopt failed";
    if (setsockopt(mWaterfallFd, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout,
                   sizeof(timeout)) < 0)
        LOG(ERROR) << "setsockopt failed";

    LOG(INFO) << "Completed acceptor";
}

WaterfallBridge* WaterfallService::getBridge() {
    return new WaterfallBridge(mLooper, mSocket, mWaterfallFd);
}
WaterfallBridge::WaterfallBridge(Looper* looper, int serverFd, int fd)
    : mLooper(looper), mSocket(serverFd), mWaterfallFd(fd) {}

std::shared_ptr<grpc::Channel> WaterfallBridge::getGrpcChannel() {
    if (mChannel) {
        return mChannel;
    }

    mChannel = openChannel();
    return mChannel;
}

int WaterfallBridge::handshake() {
    std::string msg = "pipe:unix:sockets/h2o";
    write(mWaterfallFd, msg.c_str(), msg.size() + 1);
    int fd = accept(mSocket, 0, 0);

    // Set socket timeouts to be pretty brief, so we can handshake
    // synchronously.
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if ((setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout,
                    sizeof(timeout)) < 0) ||
        (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout,
                    sizeof(timeout))) < 0) {
        LOG(ERROR) << "setsockopt failed";
        base::socketClose(fd);
        return -1;
    }

    if (base::socketSend(fd, "rdy", 3) != 3) {
        LOG(ERROR) << "Unable to send rdy";
        base::socketClose(fd);
        return -1;
    }

    char buf[8] = {0};
    int read = base::socketRecv(fd, buf, sizeof(buf));
    if (read != 3 || buf[0] != 'r' || buf[1] != 'd' || buf[2] != 'y') {
        LOG(ERROR) << "Did not receive rdy, but: [" << std::string(buf)
                   << "] len:" << read;
        base::socketClose(fd);
        return -1;
    };
    return fd;
}

void WaterfallBridge::close() {
    mDecoder->close();
    mEncoder->close();
}

std::shared_ptr<grpc::Channel> WaterfallBridge::openChannel() {
    int waterfallFd = handshake();

    if (waterfallFd < 0) {
        LOG(ERROR) << "Unable to handshake with waterfall";
        return nullptr;
    }

    int grpcAFD = 0;
    int grpcBFd = 0;
    if (base::socketCreatePair(&grpcAFD, &grpcBFd) != 0) {
        LOG(ERROR) << "Unable to create translator pair";
        return nullptr;
    }

    mWaterfallSocket = std::make_unique<AsyncSocketFd>(mLooper, waterfallFd);
    mGrpcSocket = std::make_unique<AsyncSocketFd>(mLooper, grpcAFD);
    mDecoder = std::make_unique<WaterfallDecoder>(
            mLooper, mWaterfallSocket.get(), mGrpcSocket.get());
    mEncoder = std::make_unique<WaterfallEncoder>(
            mLooper, mGrpcSocket.get(), mWaterfallSocket.get());
    mChannel = grpc::CreateCustomInsecureChannelFromFd("waterfall", grpcBFd, {});
    return mChannel;
}

}  // namespace control
}  // namespace emulation
}  // namespace android
