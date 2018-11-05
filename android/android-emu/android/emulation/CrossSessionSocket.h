#include "android/base/StringView.h"
#include "android/base/async/Looper.h"
#include "android/base/async/ScopedSocketWatch.h"
#include "android/base/sockets/ScopedSocket.h"

#include <vector>

namespace android {
namespace emulation {
class CrossSessionSocket {
public:
    CrossSessionSocket() = default;
    CrossSessionSocket(CrossSessionSocket&& other);
    CrossSessionSocket& operator=(CrossSessionSocket&& other);
    CrossSessionSocket(int fd);
    CrossSessionSocket(android::base::ScopedSocket&& socket);
    void swapSocketAndClear(android::base::ScopedSocket* socket);
    void reset();
    bool valid() const;
    // Check if a socket should be recycled for snapshot (if yes recycle it)
    static void recycleSocket(CrossSessionSocket&& socket);
    static void registerForRecycle(int socket);
    static CrossSessionSocket reclaimSocket(int fd);

    enum class DrainBehavior {
        Clear,
        AppendToBuffer
    };
    void drainSocket(DrainBehavior drainBehavior);
    bool hasStaleData() const;
    size_t readStaleData(void* data, size_t dataSize);
    size_t getReadableDataSize() const;
    void onSave(android::base::Stream* stream);
    void onLoad(android::base::Stream* stream);
    int fd() const;
private:
    android::base::ScopedSocket mSocket;
    std::vector<uint8_t> mRecvBuffer;
    int mRecvBufferBegin = 0;
    int mRecvBufferEnd = 0;
};
}
}
