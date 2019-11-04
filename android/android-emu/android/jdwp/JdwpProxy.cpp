// Copyright 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/jdwp/JdwpProxy.h"

#include "android/emulation/apacket_utils.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/threads/FunctorThread.h"
#include "android/emulation/AdbMessageSniffer.h"

#define DEBUG 2

#if DEBUG >= 1
#include <stdio.h>
#define D(...) fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#else
#define D(...) (void)0
#endif

#if DEBUG >= 2
#define DD(...) fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#else
#define DD(...) (void)0
#endif

#define _PRINT_LN fprintf(stderr, "%s: %s %d\n", __func__, __FILE__, __LINE__);

namespace android {
namespace jdwp {
void JdwpProxy::Buffer::readBytes(const AndroidPipeBuffer* buffers,
                                  int numBuffers,
                                  int* bufferIdx,
                                  size_t* bufferPst,
                                  size_t* remainBytes,
                                  size_t bytesToRead,
                                  bool skipData) {
    bytesToRead = std::min(*remainBytes, bytesToRead);
    while (bytesToRead > 0 && *bufferIdx < numBuffers) {
        size_t remainBufferSize = buffers[*bufferIdx].size - *bufferPst;
        size_t currentReadSize = std::min(bytesToRead, remainBufferSize);
        if (skipData) {
            mBytesToSkip -= currentReadSize;
        } else {
            memcpy(mBuffer + mBufferTail, buffers[*bufferIdx].data + *bufferPst,
                   currentReadSize);
            mBufferTail += currentReadSize;
        }
        *bufferPst += currentReadSize;
        bytesToRead -= currentReadSize;
        *remainBytes -= currentReadSize;
        if (*bufferPst == buffers[*bufferIdx].size) {
            (*bufferIdx)++;
            *bufferPst = 0;
        }
    }
}

#define ADB_SYNC 0x434e5953
#define ADB_CNXN 0x4e584e43
#define ADB_OPEN 0x4e45504f
#define ADB_OKAY 0x59414b4f
#define ADB_CLSE 0x45534c43
#define ADB_WRTE 0x45545257
#define ADB_AUTH 0x48545541

JdwpProxy::State JdwpProxy::nextState(State state) {
    D("State done %d", state);
    if (state < Proxying) {
        return State((int)state + 1);
    }
    return Proxying;
}

bool JdwpProxy::shouldBlockGuest() const {
    return mIsJdwp == IsJdwp::Yes && mProxyState != mClientState;
}

bool JdwpProxy::shouldRegister() const {
    return !mRegistered && mIsJdwp == IsJdwp::Yes && mProxyState == ConnectOk;
}

bool JdwpProxy::shouldCheckRegistration() const {
    return !mRegistered && mClientState == Connect;
}

bool JdwpProxy::shouldUnregister() const {
    //return mRegistered && mIsJdwp != Yes;
    return false;
}

void JdwpProxy::setRegistered(bool registered) {
    mRegistered = registered;
}

void JdwpProxy::onGuestSendData(const AndroidPipeBuffer* buffers,
                                int numBuffers,
                                size_t actualSentBytes) {
    using amessage = emulation::AdbMessageSniffer::amessage;
    if (numBuffers == 0 || actualSentBytes == 0) {
        return;
    }
    if (mIsJdwp == IsJdwp::No || mIsJdwp == IsJdwp::Unknown) {
        return;
    }
    if (mProxyState == Proxying) {
        // TODO
        return;
    }
    assert(mClientState == mProxyState);
    const size_t kHeaderSize = sizeof(amessage);
    int readBuffer = 0;
    int currentBuffer = 0;
    size_t currentPst = 0;
    while (currentBuffer < numBuffers && actualSentBytes > 0) {
        uint32_t command = 0;
        bool skipOtherCommand = false;
        switch (mProxyState) {
            case Connect:
            case HandshakeSent:
                command = ADB_OKAY;
                break;
            case HandshakeSentOk:
                command = ADB_WRTE;
                break;
            default:
                _PRINT_LN
                mIsJdwp = IsJdwp::No;
                return;
        }
        assert(mGuestSendBuffer.mBytesToSkip == 0);
        if (mGuestSendBuffer.mBufferTail < kHeaderSize) {
            mGuestSendBuffer.readBytes(
                    buffers, numBuffers, &currentBuffer, &currentPst,
                    &actualSentBytes,
                    kHeaderSize - mGuestSendBuffer.mBufferTail, false);
            if (mGuestSendBuffer.mBufferTail == kHeaderSize) {
                // check header
                amessage* msg;
                msg = (amessage*)mGuestSendBuffer.mBuffer;
                if (msg->command != command) {
                    // Bad protocol
                    _PRINT_LN
                    mIsJdwp = IsJdwp::No;
                    return;
                } else if (msg->data_length == 0) {
                    if (command == ADB_OKAY) {
                        mClientState = mProxyState = nextState(mProxyState);
                        mGuestSendBuffer.mBufferTail = 0;
                        if (mProxyState == ConnectOk) {
                            mGuestId = ((amessage*)mGuestSendBuffer.mBuffer)->arg0;
                        }
                    } else {
                        _PRINT_LN
                        mIsJdwp = IsJdwp::No;
                        return;
                    }
                } else {
                    // All jdwp connection messages are short
                    if (msg->data_length + kHeaderSize > Buffer::kBufferSize) {
                        _PRINT_LN
                        mIsJdwp = IsJdwp::No;
                        return;
                    }
                }
            }
            _PRINT_LN
        } else {
            amessage* msg;
            msg = (amessage*)mGuestSendBuffer.mBuffer;
            mGuestSendBuffer.readBytes(buffers, numBuffers, &currentBuffer,
                                       &currentPst, &actualSentBytes,
                                       kHeaderSize + msg->data_length -
                                               mGuestSendBuffer.mBufferTail,
                                       false);
            if (kHeaderSize + msg->data_length ==
                mGuestSendBuffer.mBufferTail) {
                switch (mProxyState) {
                    case HandshakeSentOk: {
                        if (msg->data_length != 14 ||
                            0 != memcmp("JDWP-Handshake",
                                        mGuestSendBuffer.mBuffer + kHeaderSize,
                                        14)) {
                            _PRINT_LN
                            mIsJdwp = IsJdwp::No;
                            return;
                        }
                        mClientState = mProxyState = nextState(mProxyState);
                        break;
                    }
                    default:
                        _PRINT_LN
                        mIsJdwp = IsJdwp::No;
                        return;
                }
            }
            _PRINT_LN
        }
    }
}

// This function track whether a connection is jdwp or not. It tracks all
// packages until it found a connection package.
void JdwpProxy::onGuestRecvData(const AndroidPipeBuffer* buffers,
                                int numBuffers,
                                size_t actualRecvBytes) {
    using amessage = emulation::AdbMessageSniffer::amessage;
    if (numBuffers == 0 || actualRecvBytes == 0) {
        return;
    }
    if (mIsJdwp == IsJdwp::No) {
        mProxyState = Uninitialized;
    }
    if (mProxyState == Proxying) {
        // TODO
        return;
    }
    assert(mClientState == mProxyState);
    const size_t kHeaderSize = sizeof(amessage);
    int readBuffer = 0;
    int currentBuffer = 0;
    size_t currentPst = 0;
    while (currentBuffer < numBuffers && actualRecvBytes > 0) {
        DD("actualRecvBytes %d", (int)actualRecvBytes);
        uint32_t command = 0;
        bool skipOtherCommand = true;
        switch (mProxyState) {
            case Uninitialized:
                command = ADB_OPEN;
                skipOtherCommand = true;
                break;
            case ConnectOk:
                command = ADB_WRTE;
                break;
            case HandshakeReplied:
                command = ADB_OKAY;
                break;
            default:
                _PRINT_LN
                mIsJdwp = IsJdwp::No;
                return;
        }
        if (mGuestRecvBuffer.mBytesToSkip) {
            mGuestRecvBuffer.readBytes(buffers, numBuffers, &currentBuffer,
                                       &currentPst, &actualRecvBytes,
                                       mGuestRecvBuffer.mBytesToSkip, true);
        } else if (mGuestRecvBuffer.mBufferTail < kHeaderSize) {
            mGuestRecvBuffer.readBytes(
                    buffers, numBuffers, &currentBuffer, &currentPst,
                    &actualRecvBytes,
                    kHeaderSize - mGuestRecvBuffer.mBufferTail, false);
            if (mGuestRecvBuffer.mBufferTail == kHeaderSize) {
                // check header
                amessage* msg;
                msg = (amessage*)mGuestRecvBuffer.mBuffer;
                if (msg->command != command) {
                    if (skipOtherCommand) {
                        // Skip payload if not ADB_OPEN
                        mGuestRecvBuffer.mBytesToSkip = msg->data_length;
                        mGuestRecvBuffer.mBufferTail = 0;
                    } else {
                        // Bad protocol
                        _PRINT_LN
                        mIsJdwp = IsJdwp::No;
                        continue;
                    }
                } else if (msg->data_length == 0) {
                    if (command == ADB_OKAY) {
                        mClientState = mProxyState = nextState(mProxyState);
                        mGuestRecvBuffer.mBufferTail = 0;
                    } else {
                        _PRINT_LN
                        mGuestRecvBuffer.mBufferTail = 0;
                        mIsJdwp = IsJdwp::No;
                        continue;
                    }
                } else {  // open or handshake
                    if (msg->data_length + kHeaderSize > Buffer::kBufferSize) {
                        // The JDWP open and handshake are super short.
                        // If we got a long message, we know it is not JDWP
                        _PRINT_LN
                        mIsJdwp = IsJdwp::No;
                        mGuestRecvBuffer.mBytesToSkip = msg->data_length;
                        mGuestRecvBuffer.mBufferTail = 0;
                        continue;
                    }
                }
            }
        } else {
            amessage* msg;
            msg = (amessage*)mGuestRecvBuffer.mBuffer;
            mGuestRecvBuffer.readBytes(buffers, numBuffers, &currentBuffer,
                                       &currentPst, &actualRecvBytes,
                                       kHeaderSize + msg->data_length -
                                               mGuestRecvBuffer.mBufferTail,
                                       false);
            if (kHeaderSize + msg->data_length ==
                mGuestRecvBuffer.mBufferTail) {
                switch (mProxyState) {
                    case Uninitialized: {
                        int jdwpConnect =
                                sscanf(mGuestRecvBuffer.mBuffer + kHeaderSize,
                                       "jdwp:%d", &mGuestPid);
                        if (jdwpConnect == 0) {
                            _PRINT_LN
                            mIsJdwp = IsJdwp::No;
                            break;
                            //return;
                        }
                        mIsJdwp = IsJdwp::Yes;
                        mHostId = ((amessage*)mGuestRecvBuffer.mBuffer)->arg0;
                        D("Found jdwp for pid %d", mGuestPid);
                        mClientState = mProxyState = nextState(mProxyState);
                        break;
                    }
                    case ConnectOk: {
                        if (msg->data_length != 14 ||
                            0 != memcmp("JDWP-Handshake",
                                        mGuestRecvBuffer.mBuffer + kHeaderSize,
                                        14)) {
                            _PRINT_LN
                            mIsJdwp = IsJdwp::No;
                            break;
                            //return;
                        }
                        mClientState = mProxyState = nextState(mProxyState);
                        break;
                    }
                    default:
                        _PRINT_LN
                        mIsJdwp = IsJdwp::No;
                        //return;
                }
                mGuestRecvBuffer.mBufferTail = 0;
            }
        }
    }
}

void JdwpProxy::fakeServerAsync(int socket,
        std::function<void()> serverDoneCallback) {
    if (mProxyState == mClientState) {
        mFakeServerThread.reset();
    } else {
        mFakeServerThread.reset(new base::FunctorThread(
                [this, socket, serverDoneCallback]() -> intptr_t {
            base::socketSetBlocking(socket);
            while (mProxyState != mClientState) {
                assert(mProxyState > mClientState);
                emulation::apacket toHost;
                toHost.mesg.arg0 = mGuestId;
                toHost.mesg.arg1 = mHostId;
                toHost.mesg.data_check = 0;
                toHost.mesg.data_length = 0;
                switch (mClientState) {
                    case Connect:
                    case HandshakeSent:
                        toHost.mesg.command = ADB_OKAY;
                        toHost.mesg.magic = toHost.mesg.command ^ 0xffffffff;
                        if (!emulation::sendPacket(socket, &toHost)) {
                            _PRINT_LN
                            return intptr_t();
                        }
                        break;
                    case ConnectOk: {
                        emulation::apacket toGuest;
                        if (!emulation::recvPacket(socket, &toGuest)) {
                            _PRINT_LN
                            return intptr_t();
                        }
                        if (toGuest.mesg.command != ADB_WRTE || toGuest.mesg.data_length != 14
                                || memcmp(toGuest.data.data(), "JDWP-Handshake", 14) != 0) {
                            // TODO: do we need to close the socket?
                            _PRINT_LN
                            return intptr_t();
                        }
                        break;
                    }
                    case HandshakeSentOk:
                        toHost.mesg.command = ADB_WRTE;
                        toHost.mesg.magic = toHost.mesg.command ^ 0xffffffff;
                        toHost.mesg.data_length = 14;
                        toHost.data.resize(14);
                        memcpy(toHost.data.data(), "JDWP-Handshake", 14);
                        if (!emulation::sendPacket(socket, &toHost)) {
                            _PRINT_LN
                            return intptr_t();
                        }
                        break;
                    case HandshakeReplied:
                        if (!emulation::recvOkay(socket)) {
                            _PRINT_LN
                            return intptr_t();
                        }
                        break;
                    case Uninitialized:
                    case HandshakeRepliedOk:
                        _PRINT_LN
                        assert(false);
                        break;
                }
                mClientState = nextState(mClientState);
            }
            D("Fake JDWP server done, proxying to real jdwp");
            base::socketSetNonBlocking(socket);
            serverDoneCallback();
            return intptr_t();
        }));
        mFakeServerThread->start();
    }
}

void JdwpProxy::resetServerThread() {
    mFakeServerThread.reset();
}

}  // namespace jdwp
}  // namespace android
