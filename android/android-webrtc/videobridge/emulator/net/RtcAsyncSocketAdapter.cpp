#include "emulator/net/RtcAsyncSocketAdapter.h"
#include <chrono>
using rtc::AsyncSocket;

namespace emulator {
namespace net {

RtcAsyncSocketAdapter::RtcAsyncSocketAdapter(AsyncSocket* rtcSocket)
    : mRtcSocket(rtcSocket) {
    mRtcSocket->SignalCloseEvent.connect(this, &RtcAsyncSocketAdapter::onClose);
    mRtcSocket->SignalReadEvent.connect(this, &RtcAsyncSocketAdapter::onRead);
    mRtcSocket->SignalConnectEvent.connect(this, &RtcAsyncSocketAdapter::onConnected);
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

void RtcAsyncSocketAdapter::onConnected(rtc::AsyncSocket* socket) {

    std::lock_guard<std::mutex> guard(mConnectLock);
    if (mListener) {
        mListener->onConnected(this);
    }
    mConnectCv.notify_one();
}

void RtcAsyncSocketAdapter::dispose() {
    setSocketEventListener(nullptr);
    close();
}

bool RtcAsyncSocketAdapter::connect() {
    return mRtcSocket->Connect(mRtcSocket->GetRemoteAddress());
}

bool RtcAsyncSocketAdapter::connected() {
    return mRtcSocket->GetState() == rtc::AsyncSocket::ConnState::CS_CONNECTED;
}

bool RtcAsyncSocketAdapter::connectSync(uint64_t timeoutms) {
    bool connect = this->connect();
    if (!connect)
        return false;

    std::unique_lock<std::mutex> guard(mConnectLock);
    std::chrono::system_clock::time_point now =
            std::chrono::system_clock::now();
    return mConnectCv.wait_until(guard, now +  std::chrono::milliseconds(timeoutms), [this](){return connected();});

}

}  // namespace net
}  // namespace emulator