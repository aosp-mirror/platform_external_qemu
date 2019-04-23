
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

    void AddSocketEventListener(AsyncSocketEventListener* listener);
    void RemoveSocketEventListener(AsyncSocketEventListener* listener) override;

    void Close() override;
    uint64_t Recv(char* buffer, uint64_t bufferSize) override;
    uint64_t Send(const char* buffer, uint64_t bufferSize) override;

private:
    void OnRead(rtc::AsyncSocket* socket);
    void OnClose(rtc::AsyncSocket* socket, int err);

    std::unique_ptr<rtc::AsyncSocket> mRtcSocket;
    AsyncSocketEventListener* mListener;
};
}  // namespace net
}  // namespace emulator