#include "SocketTransport.h"
#include <rtc_base/logging.h>

using rtc::LoggingSeverity;
namespace emulator {
namespace net {
std::ostream& operator<<(std::ostream& os, const State state) {
    switch (state) {
        case State::NOT_CONNECTED:
            os << "NOT_CONNECTED";
        break;
        case State::RESOLVING:
            os << "RESOLVING";
        break;
        case State::SIGNING_IN:
            os << "SIGNING_IN";
        break;
        case State::CONNECTED:
            os << "CONNECTED";
        break;
    }
    return os;
}

SocketTransport::SocketTransport(MessageReceiver* receiver) : mReceiver(receiver) {}
SocketTransport::~SocketTransport() {}

void SocketTransport::OnClose(rtc::AsyncSocket* socket, int err) {
    socket->Close();

#ifdef WIN32
    if (err != WSAECONNREFUSED) {
#else
    if (err != ECONNREFUSED) {
#endif
        if (socket == mSocket.get()) {
            // rtc::Thread::Current()->PostDelayed(RTC_FROM_HERE,
            // kReconnectDelay, this, 0); Reconnect?
        } else {
            close();
        }
    }
}

static rtc::AsyncSocket* CreateClientSocket(int family) {
#ifdef WIN32
    rtc::Win32Socket* sock = new rtc::Win32Socket();
    sock->CreateT(family, SOCK_STREAM);
    return sock;
#elif defined(WEBRTC_POSIX)
    rtc::Thread* thread = rtc::Thread::Current();
    RTC_DCHECK(thread != NULL);
    return thread->socketserver()->CreateAsyncSocket(family, SOCK_STREAM);
#else
#error Platform not supported.
#endif
}

void SocketTransport::setState(State newState) {
    RTC_LOG(INFO) << (int) mState << " -> " << (int) newState;
    mState = newState;
    if (mReceiver) {
        mReceiver->stateConnectionChange(this, mState);
    }
}

void SocketTransport::doConnect() {
    mSocket.reset(CreateClientSocket(mAddress.ipaddr().family()));
    mSocket->SignalCloseEvent.connect(this, &SocketTransport::OnClose);
    mSocket->SignalConnectEvent.connect(this, &SocketTransport::OnConnect);
    mSocket->SignalReadEvent.connect(this, &SocketTransport::OnRead);
    setState(State::SIGNING_IN);
    int err = mSocket->Connect(mAddress);
    if (err == SOCKET_ERROR) {
        RTC_LOG(LERROR) << "Unable to connect to " << mAddress.HostAsURIString();
        close();
    }
}

void SocketTransport::connect(const std::string server, int port) {
    RTC_DCHECK(!server.empty());
    RTC_LOG(INFO) << "Connection to " << server.c_str() << ":" << port;

    if (mState != State::NOT_CONNECTED) {
        RTC_LOG(WARNING) << "Already connected";
        return;
    }

    mAddress.SetIP(server);
    mAddress.SetPort(port);

    if (mAddress.IsUnresolvedIP()) {
        // TODO(jansene): This does not seem to work right now..
        RTC_LOG(INFO) << "Resolving IP:" << mAddress.HostAsURIString();
        setState(State::RESOLVING);
        mResolver.reset(new rtc::AsyncResolver());
        mResolver->SignalDone.connect(this, &SocketTransport::OnResolveResult);
        mResolver->Start(mAddress);
    } else {
        doConnect();
    }
}

void SocketTransport::OnResolveResult(rtc::AsyncResolverInterface* resolver) {
    if (resolver->GetError() != 0) {
        RTC_LOG(LERROR) << "Unable to resolve address, err: "
                    << resolver->GetError();
        setState(State::NOT_CONNECTED);
    } else {
        mAddress = resolver->address();
        RTC_LOG(INFO) << "resolved address: " << mAddress.HostAsURIString() << ", connecting now";
        doConnect();
    }
}

void SocketTransport::close() {
    mSocket->Close();
    if (mResolver) {
        mResolver.release()->Destroy(false);
    }
    setState(State::NOT_CONNECTED);
}

void SocketTransport::OnConnect(rtc::AsyncSocket* socket) {
    setState(State::CONNECTED);
}
bool SocketTransport::send(const std::string msg) {
    return mSocket->Send(msg.c_str(), msg.size()) > 0;
}

void SocketTransport::OnRead(rtc::AsyncSocket* socket) {
    char buffer[0xffff];
    do {
        int bytes = socket->Recv(buffer, sizeof(buffer), nullptr);
        if (bytes <= 0) {
            break;
        }

        mReceiver->received(this, std::string(buffer, bytes));
    } while (true);
}
}
}
