#include "emulator/net/RtcAsyncSocketAdapter.h"
using rtc::AsyncSocket;

namespace emulator {
namespace net {

RtcAsyncSocketAdapter::RtcAsyncSocketAdapter(AsyncSocket* rtcSocket)
    : mRtcSocket(rtcSocket) {
    mRtcSocket->SignalCloseEvent.connect(this, &RtcAsyncSocketAdapter::OnClose);
    mRtcSocket->SignalReadEvent.connect(this, &RtcAsyncSocketAdapter::OnRead);
}

RtcAsyncSocketAdapter::~RtcAsyncSocketAdapter() {
    Close();
};

void RtcAsyncSocketAdapter::AddSocketEventListener(
        AsyncSocketEventListener* listener)  {
    mListener = listener;
}

void RtcAsyncSocketAdapter::RemoveSocketEventListener(
        AsyncSocketEventListener* listener)  {
    mListener = nullptr;
};

void RtcAsyncSocketAdapter::Close()  {
    mRtcSocket->Close();
};

uint64_t RtcAsyncSocketAdapter::Recv(char* buffer,
                                     uint64_t bufferSize)  {
    return mRtcSocket->Recv(buffer, bufferSize, nullptr);
}

uint64_t RtcAsyncSocketAdapter::Send(const char* buffer,
                                     uint64_t bufferSize)  {
    return mRtcSocket->Send(buffer, bufferSize);
}

void RtcAsyncSocketAdapter::OnRead(rtc::AsyncSocket* socket) {
    if (mListener) {
        mListener->OnRead(this);
    }
}
void RtcAsyncSocketAdapter::OnClose(rtc::AsyncSocket* socket, int err) {
    if (mListener) {
        mListener->OnClose(this, err);
    }
}

}  // namespace net
}  // namespace emulator