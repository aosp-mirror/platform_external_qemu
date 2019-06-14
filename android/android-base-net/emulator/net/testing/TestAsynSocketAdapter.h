#include <string>
#include <vector>

#include "android/base/synchronization/Lock.h"
#include "emulator/net/AsyncSocketAdapter.h"
#include "emulator/net/SocketTransport.h"

namespace emulator {
namespace net {
using android::base::AutoLock;
using android::base::Lock;

template <class T>
class SimpleProtocolReader : public ProtocolReader<T> {
public:
    void received(SocketTransport* from, T object) override {
        mReceived.push_back(object);
    }
    void stateConnectionChange(SocketTransport* connection,
                               State current) override {
        mCurrent = current;
    }

    std::vector<T> mReceived;
    State mCurrent;
};

// A class that can be used to test sockets.
class TestAsyncSocketAdapter : public AsyncSocketAdapter {
public:
    TestAsyncSocketAdapter(std::vector<std::string> recv) : mRecv(recv) {}
    ~TestAsyncSocketAdapter() = default;
    void close() override {
        notify(Notification::Close, 123);
        mConnected = false;
    }

    uint64_t recv(char* buffer, uint64_t bufferSize) override {
        AutoLock lock(mReadLock);
        if (mRecv.empty()) {
            return 0;
        }
        std::string msg = mRecv.front();
        // Note that c_str() is guaranteed to be 0 terminated, so we always
        // have size() + 1 of in our string.
        uint64_t buflen = std::min<uint64_t>(bufferSize, msg.size() + 1);
        memcpy(buffer, msg.c_str(), buflen);
        mRecv.erase(mRecv.begin(), mRecv.begin() + 1);
        return buflen;
    }
    uint64_t send(const char* buffer, uint64_t bufferSize) override {
        AutoLock sendLock(mSendLock);
        std::string msg(buffer, bufferSize);
        mSend.push_back(msg);
        return bufferSize;
    }

    bool connect() override {
        mConnected = true;
        return true;
    }

    bool connected() override { return mConnected; }

    // This will push out all the messages.
    void signalRecv() { notify(Notification::Read); }

    // This will push all the send messages back to itself.
    void loopback() {
        for(auto msg : mSend) {
            mRecv.push_back(msg);
        }
        signalRecv();
    }

    // Signal the arrival of a new message..
    void signalRecv(std::string msg) {
        {
            AutoLock lock(mReadLock);
            mRecv.push_back(msg);
        }
        signalRecv();
    }

    std::vector<std::string> mSend;
    std::vector<std::string> mRecv;
    bool mConnected = true;
    Lock mReadLock;
    Lock mSendLock;
};
}  // namespace net
}  // namespace emulator
