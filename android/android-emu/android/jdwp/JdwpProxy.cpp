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

JdwpProxy::Status JdwpProxy::nextStatus(Status status) {
    D("State done %d", status);
    if (status < Proxying) {
        return Status((int)status + 1);
    }
    return Proxying;
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
    if (mProxyStatus == Proxying) {
        _PRINT_LN
        // TODO
        return;
    }
    const size_t kHeaderSize = sizeof(amessage);
    int readBuffer = 0;
    int currentBuffer = 0;
    size_t currentPst = 0;
    while (currentBuffer < numBuffers && actualSentBytes > 0) {
        uint32_t command = 0;
        bool skipOtherCommand = false;
        switch (mProxyStatus) {
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
                        mProxyStatus = nextStatus(mProxyStatus);
                        mGuestSendBuffer.mBufferTail = 0;
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
                switch (mProxyStatus) {
                    case HandshakeSentOk: {
                        if (msg->data_length != 14 ||
                            0 != memcmp("JDWP-Handshake",
                                        mGuestSendBuffer.mBuffer + kHeaderSize,
                                        14)) {
                            _PRINT_LN
                            mIsJdwp = IsJdwp::No;
                            return;
                        }
                        mProxyStatus = nextStatus(mProxyStatus);
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
        return;
    }
    if (mProxyStatus == Proxying) {
        // TODO
        return;
    }
    const size_t kHeaderSize = sizeof(amessage);
    int readBuffer = 0;
    int currentBuffer = 0;
    size_t currentPst = 0;
    while (currentBuffer < numBuffers && actualRecvBytes > 0) {
        DD("actualRecvBytes %d", (int)actualRecvBytes);
        uint32_t command = 0;
        bool skipOtherCommand = false;
        switch (mProxyStatus) {
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
                        return;
                    }
                } else if (msg->data_length == 0) {
                    if (command == ADB_OKAY) {
                        mProxyStatus = nextStatus(mProxyStatus);
                        mGuestRecvBuffer.mBufferTail = 0;
                    } else {
                        _PRINT_LN
                        mIsJdwp = IsJdwp::No;
                        return;
                    }
                } else {  // open or handshake
                    if (msg->data_length + kHeaderSize > Buffer::kBufferSize) {
                        // The JDWP open and handshake are super short.
                        // If we got a long message, we know it is not JDWP
                        _PRINT_LN
                        mIsJdwp = IsJdwp::No;
                        return;
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
                switch (mProxyStatus) {
                    case Uninitialized: {
                        int jdwpConnect =
                                sscanf(mGuestRecvBuffer.mBuffer + kHeaderSize,
                                       "jdwp:%d", &mGuestPid);
                        if (jdwpConnect == 0) {
                            _PRINT_LN
                            mIsJdwp = IsJdwp::No;
                            return;
                        }
                        mIsJdwp = IsJdwp::Yes;
                        D("Found jdwp for pid %d", mGuestPid);
                        mProxyStatus = nextStatus(mProxyStatus);
                        break;
                    }
                    case ConnectOk: {
                        if (msg->data_length != 14 ||
                            0 != memcmp("JDWP-Handshake",
                                        mGuestRecvBuffer.mBuffer + kHeaderSize,
                                        14)) {
                            _PRINT_LN
                            mIsJdwp = IsJdwp::No;
                            return;
                        }
                        mProxyStatus = nextStatus(mProxyStatus);
                        break;
                    }
                    default:
                        _PRINT_LN
                        mIsJdwp = IsJdwp::No;
                        return;
                }
                mGuestRecvBuffer.mBufferTail = 0;
            }
        }
    }
    // printf("current buffer %d pst %d num buffer %d\n", (int)currentBuffer,
    // (int)currentPst, numBuffers);
    _PRINT_LN
}

}  // namespace jdwp
}  // namespace android
