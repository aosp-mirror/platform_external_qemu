
#include <rtc_base/third_party/sigslot/sigslot.h>  // for has_slots
#include <stdint.h>                                // for uint64_t
#include <memory>                                  // for unique_ptr
#include <mutex>                                   // for condition_variable

#include "emulator/net/AsyncSocketAdapter.h"       // for AsyncSocketAdapter

namespace rtc {
class AsyncSocket;
}  // namespace rtc

using rtc::AsyncSocket;

namespace emulator {
namespace net {

// A connected asynchronous socket.
// The videobridge will provide an implementation, as well as the android-webrtc
// module.
class RtcAsyncSocketAdapter : public AsyncSocketAdapter,
                              public sigslot::has_slots<> {
public:
    RtcAsyncSocketAdapter(AsyncSocket* rtcSocket);
    ~RtcAsyncSocketAdapter();

    void close() override;
    uint64_t recv(char* buffer, uint64_t bufferSize) override;
    uint64_t send(const char* buffer, uint64_t bufferSize) override;
    bool connected() override;
    bool connectSync(uint64_t timeoutms) override;
    void dispose() override;

    bool connect() override;

private:
    void onRead(rtc::AsyncSocket* socket);
    void onClose(rtc::AsyncSocket* socket, int err);
    void onConnected(rtc::AsyncSocket* socket);

    std::unique_ptr<rtc::AsyncSocket> mRtcSocket;
    std::mutex mConnectLock;
    std::condition_variable mConnectCv;
};
}  // namespace net
}  // namespace emulator
