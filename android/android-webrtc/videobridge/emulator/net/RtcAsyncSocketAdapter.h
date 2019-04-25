
#include <rtc_base/nethelpers.h>
#include <rtc_base/socketaddress.h>

#include <string>

#include "emulator/net/AsyncSocketAdapter.h"
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
    bool connect() override;

private:
    void onRead(rtc::AsyncSocket* socket);
    void onClose(rtc::AsyncSocket* socket, int err);

    std::unique_ptr<rtc::AsyncSocket> mRtcSocket;
};
}  // namespace net
}  // namespace emulator