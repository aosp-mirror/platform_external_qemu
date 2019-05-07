#include "emulator/net/RtcAsyncSocketAdapter.h"
using rtc::AsyncSocket;

namespace emulator {
namespace net {

RtcAsyncSocketAdapter::RtcAsyncSocketAdapter(AsyncSocket* rtcSocket)
    : mRtcSocket(rtcSocket) {
    mRtcSocket->SignalCloseEvent.connect(this, &RtcAsyncSocketAdapter::onClose);
    mRtcSocket->SignalReadEvent.connect(this, &RtcAsyncSocketAdapter::onRead);
}

RtcAsyncSocketAdapter::~RtcAsyncSocketAdapter() {
    close();
};

void RtcAsyncSocketAdapter::close() {
    mRtcSocket->Close();
};

uint64_t RtcAsyncSocketAdapter::recv(char* buffer, uint64_t bufferSize) {
    return mRtcSocket->Recv(buffer, bufferSize, nullptr);
}

uint64_t RtcAsyncSocketAdapter::send(const char* buffer, uint64_t bufferSize) {
    return mRtcSocket->Send(buffer, bufferSize);
}

void RtcAsyncSocketAdapter::onRead(rtc::AsyncSocket* socket) {
    if (mListener) {
        mListener->onRead(this);
    }
}
void RtcAsyncSocketAdapter::onClose(rtc::AsyncSocket* socket, int err) {
    if (mListener) {
        mListener->onClose(this, err);
    }
}

bool RtcAsyncSocketAdapter::connect() {
    return mRtcSocket->Connect(mRtcSocket->GetRemoteAddress());
}

bool RtcAsyncSocketAdapter::connected() {
    return mRtcSocket->GetState() == rtc::AsyncSocket::ConnState::CS_CONNECTED;
}

}  // namespace net
}  // namespace emulator