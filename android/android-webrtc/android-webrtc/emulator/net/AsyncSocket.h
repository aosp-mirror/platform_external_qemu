#include "android/base/Log.h"
#include "android/base/async/AsyncWriter.h"
#include "android/base/async/Looper.h"
#include "android/base/containers/BufferQueue.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/synchronization/Lock.h"
#include "emulator/net/AsyncSocketAdapter.h"

namespace emulator {
namespace net {
using MessageQueue = android::base::BufferQueue<std::string>;
using android::base::Looper;
using android::base::Lock;

class AsyncSocket : public AsyncSocketAdapter {
public:
    AsyncSocket(Looper* looper);
    ~AsyncSocket() = default;
    void AddSocketEventListener(AsyncSocketEventListener* listener) override;
    void RemoveSocketEventListener(AsyncSocketEventListener* listener) override;
    void Close() override;
    uint64_t Recv(char* buffer, uint64_t bufferSize) override;
    uint64_t Send(const char* buffer, uint64_t bufferSize) override;

    bool loopbackConnect(int port);
    bool connected() const;

    void onWrite();
    void onRead();

private:
    static const int DEFAULT_READ_BUFFER_SIZE = 1024;
    static const int WRITE_BUFFER_SIZE = 1024;

    int mSocket;
    Looper* mLooper;
    std::unique_ptr<Looper::FdWatch> mFdWatch;

    ::android::base::AsyncWriter mAsyncWriter;
    MessageQueue mWriteQueue;
    Lock mLock;
    std::string mWriteBuffer;

    AsyncSocketEventListener* mListener = nullptr;
};
}  // namespace net
}  // namespace emulator
