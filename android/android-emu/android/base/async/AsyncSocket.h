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
using MessageQueue = android::base::BufferQueue<std::string>;
using emulator::net::AsyncSocketAdapter;

// An AsyncSocket is a socket that can connect to a local port on
// the current machine.
class AsyncSocket : public AsyncSocketAdapter {
public:
    AsyncSocket(Looper* looper, int port);
    ~AsyncSocket() = default;
    void close() override;
    uint64_t recv(char* buffer, uint64_t bufferSize) override;
    uint64_t send(const char* buffer, uint64_t bufferSize) override;

    bool connect() override;
    bool connected() override;

    void onWrite();
    void onRead();

private:
    void connectToPort();
    static const int WRITE_BUFFER_SIZE = 1024;

    int mSocket;
    int mPort;

    Looper* mLooper;
    bool mConnecting = false;

    std::unique_ptr<Looper::FdWatch> mFdWatch;
    ::android::base::AsyncWriter mAsyncWriter;
    ::android::base::FunctorThread mConnectThread;

    // Queue of message that need to go out over this socket.
    MessageQueue mWriteQueue;
    Lock mWriteQueueLock;
    Lock mWatchLock;

    // Write buffer used by the async writer.
    std::string mWriteBuffer;
};
}  // namespace base
}  // namespace android