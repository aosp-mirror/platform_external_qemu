#include "android/base/Log.h"
#include "android/base/async/AsyncWriter.h"
#include "android/base/async/Looper.h"
#include "android/base/containers/BufferQueue.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/threads/FunctorThread.h"
#include "emulator/net/AsyncSocketAdapter.h"

namespace android {
namespace base {
using MessageQueue = BufferQueue<std::string>;
using emulator::net::AsyncSocketAdapter;
using emulator::net::AsyncSocketEventListener;

// An AsyncSocket wraps an existing socket.
class AsyncSocketFd : public AsyncSocketAdapter {
public:
    enum class SocketMode: unsigned { Read = 0x1, Write = 0x2 };
    AsyncSocketFd(Looper* looper,
                  int fd,
                  SocketMode mode = SocketMode::Read | SocketMode::Write);
    ~AsyncSocketFd();
    void close() override;
    uint64_t recv(char* buffer, uint64_t bufferSize) override;
    uint64_t send(const char* buffer, uint64_t bufferSize) override;
    bool connected() override;
    virtual bool connect();
    void onWrite();
    void onRead();

protected:

    SocketMode mMode;
    int mSocket;
    Lock mWatchLock;
    std::unique_ptr<Looper::FdWatch> mFdWatch;
    Looper* mLooper;

private:
    static const int WRITE_BUFFER_SIZE = 1024;
    ::android::base::AsyncWriter mAsyncWriter;

    // Queue of message that need to go out over this socket.
    MessageQueue mWriteQueue;
    Lock mWriteQueueLock;

    // Write buffer used by the async writer.
    std::string mWriteBuffer;
};

// An AsyncSocket is a socket that can connect to a local port on
// the current machine.
class AsyncSocket : public AsyncSocketFd {
public:
    AsyncSocket(Looper* looper, int port);
    bool connect() override;

private:
    void connectToPort();

    bool mConnecting = false;
    int mPort;
    ::android::base::FunctorThread mConnectThread;
};
}  // namespace base
}  // namespace android
