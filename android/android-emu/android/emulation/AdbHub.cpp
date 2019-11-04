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

#include "android/emulation/AdbHub.h"

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
namespace emulation {
void AdbHub::Buffer::readBytes(const AndroidPipeBuffer* buffers,
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
            if (mBufferTail + currentReadSize > mBuffer.size()) {
                mBuffer.resize(mBufferTail + currentReadSize);
            }
            memcpy(mBuffer.data() + mBufferTail, buffers[*bufferIdx].data + *bufferPst,
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

void AdbHub::Buffer::onSave(android::base::Stream *stream) {
    stream->putBe64(mBufferTail);
    if (mBufferTail > 0) {
        stream->write(mBuffer.data(), mBufferTail);
    }
    stream->putBe64(mBytesToSkip);
}

void AdbHub::Buffer::onLoad(android::base::Stream *stream) {
    mBufferTail = stream->getBe64();
    if (mBufferTail > 0) {
        if (mBufferTail > mBuffer.size()) {
            mBuffer.resize(mBufferTail);
        }
        stream->read(mBuffer.data(), mBufferTail);
    }
    mBytesToSkip = stream->getBe64();
}

void AdbHub::onSave(android::base::Stream *stream) {
    if (mCurrentRecvProxy == mProxies.end()) {
        stream->putBe32(-1);
    } else {
        stream->putBe32(mCurrentRecvProxy->first);
    }
    if (mCurrentSendProxy == mProxies.end()) {
        stream->putBe32(-1);
    } else {
        stream->putBe32(mCurrentSendProxy->first);
    }
}

void AdbHub::onLoad(android::base::Stream *stream) {
    mLoadRecvProxyId = stream->getBe32();
    mLoadSendProxyId = stream->getBe32();
    mCurrentRecvProxy = mProxies.end();
    mCurrentSendProxy = mProxies.end();
}

void AdbHub::insertProxy(AdbProxy *proxy) {
    mProxies.emplace(proxy->guestId(), proxy);
}

void AdbHub::doneInsertProxy() {
    if (mLoadRecvProxyId >= 0) {
        mCurrentRecvProxy = mProxies.find(mLoadRecvProxyId);
        mLoadRecvProxyId = -1;
    }
    if (mLoadSendProxyId >= 0) {
        mCurrentSendProxy = mProxies.find(mLoadSendProxyId);
        mLoadSendProxyId = -1;
    }
}

void AdbHub::checkCloseProxy(std::unordered_map<int, AdbProxy *>::iterator proxy) {
    if (proxy != mProxies.end() && proxy->second->shouldClose()) {
        mProxies.erase(proxy);
    }
}

int AdbHub::onGuestSendData(const AndroidPipeBuffer* buffers,
                                int numBuffers,
                                OnNewConnection onNewConnection) {
    int actualSentBytesI = 0;
    if (mNeedTrasnalte) {
        for (int i = 0; i < numBuffers; i++) {
            actualSentBytesI += buffers[i].size;
        }
    } else {
        actualSentBytesI = sendToHost(buffers, numBuffers);
        if (actualSentBytesI <= 0) {
            return actualSentBytesI;
        }
    }
    size_t bytesToParse = (size_t)actualSentBytesI;
    using amessage = emulation::AdbMessageSniffer::amessage;
    assert(numBuffers != 0 && bytesToParse != 0);
    const size_t kHeaderSize = sizeof(amessage);
    int readBuffer = 0;
    int currentBuffer = 0;
    size_t currentPst = 0;
    while (currentBuffer < numBuffers && bytesToParse > 0) {
        if (mGuestSendBuffer.mBytesToSkip) {
            mGuestSendBuffer.readBytes(buffers, numBuffers, &currentBuffer,
                                        &currentPst, &bytesToParse,
                                        mGuestSendBuffer.mBytesToSkip, true);
        } else if (mGuestSendBuffer.mBufferTail < kHeaderSize) {
            mGuestSendBuffer.readBytes(
                    buffers, numBuffers, &currentBuffer, &currentPst,
                    &bytesToParse,
                    kHeaderSize - mGuestSendBuffer.mBufferTail, false);
            if (mGuestSendBuffer.mBufferTail == kHeaderSize) {
                // check header
                amessage* mesg;
                mesg = (amessage*)mGuestSendBuffer.mBuffer.data();
                auto pendingConnection = mPendingConnections.find(mesg->arg1);
                if (mesg->command == ADB_OKAY && pendingConnection != mPendingConnections.end()) {
                    AdbProxy* newProxy = onNewConnection(pendingConnection->second, mesg->arg0);
                    if (newProxy) {
                        mProxies.emplace(mesg->arg0, newProxy);
                    }
                    mPendingConnections.erase(pendingConnection);
                    mGuestSendBuffer.mBufferTail = 0;
                } else {
                    auto proxyIte = mProxies.find(mesg->arg0);
                    if (proxyIte == mProxies.end() ||
                        !proxyIte->second->needProxySendData(mesg) ||
                        mesg->data_length == 0) {
                        mGuestSendBuffer.mBytesToSkip = mesg->data_length;
                        mGuestSendBuffer.mBufferTail = 0;
                        mCurrentSendProxy = mProxies.end();
                    } else {
                        mCurrentSendProxy = proxyIte;
                    }
                    checkCloseProxy(proxyIte);
                }
            }
        } else {
            amessage* mesg;
            mesg = (amessage*)mGuestSendBuffer.mBuffer.data();
            mGuestSendBuffer.readBytes(buffers, numBuffers, &currentBuffer,
                                       &currentPst, &bytesToParse,
                                       kHeaderSize + mesg->data_length -
                                               mGuestSendBuffer.mBufferTail,
                                       false);
            // could be resized
            mesg = (amessage*)mGuestSendBuffer.mBuffer.data();
            if (kHeaderSize + mesg->data_length == mGuestSendBuffer.mBufferTail) {
                mCurrentSendProxy->second->onGuestSendData(mGuestSendBuffer.mBuffer.data(),
                        mGuestSendBuffer.mBuffer.size());
                checkCloseProxy(mCurrentSendProxy);
                mCurrentSendProxy = mProxies.end();
                mGuestSendBuffer.mBufferTail = 0;
            }
        }
    }
    return actualSentBytesI;
}

// This function track whether a connection is jdwp or not. It tracks all
// packages until it found a connection package.
void AdbHub::onGuestRecvData(const AndroidPipeBuffer* buffers,
                                int numBuffers,
                                size_t actualRecvBytes,
                                OnNewConnection onNewConnection) {
    using amessage = emulation::AdbMessageSniffer::amessage;
    if (numBuffers == 0 || actualRecvBytes == 0) {
        return;
    }
    const size_t kHeaderSize = sizeof(amessage);
    int readBuffer = 0;
    int currentBuffer = 0;
    size_t currentPst = 0;
    while (currentBuffer < numBuffers && actualRecvBytes > 0) {
        DD("actualRecvBytes %d", (int)actualRecvBytes);
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
                amessage* mesg = (amessage*)mGuestRecvBuffer.mBuffer.data();
                if (mesg->command == ADB_OPEN) {
                    DD("New adb open");
                    mCurrentRecvProxy = mProxies.end();
                } else {
                    auto proxyIte = mProxies.find(mesg->arg1);
                    if (proxyIte == mProxies.end() ||
                        !proxyIte->second->needProxyRecvData(mesg) ||
                        mesg->data_length == 0) {
                        mGuestRecvBuffer.mBytesToSkip = mesg->data_length;
                        mGuestRecvBuffer.mBufferTail = 0;
                        mCurrentRecvProxy = mProxies.end();
                        DD("Skip %d data for pipe %d", (int)mGuestRecvBuffer.mBytesToSkip, mesg->arg0);
                    } else {
                        DD("Parse data for pipe %d", mesg->arg0);
                        mCurrentRecvProxy = proxyIte;
                    }
                    checkCloseProxy(proxyIte);
                }
            }
        } else {
            amessage* mesg;
            mesg = (amessage*)mGuestRecvBuffer.mBuffer.data();
            mGuestRecvBuffer.readBytes(buffers, numBuffers, &currentBuffer,
                                       &currentPst, &actualRecvBytes,
                                       kHeaderSize + mesg->data_length -
                                               mGuestRecvBuffer.mBufferTail,
                                       false);
            // could be resized
            mesg = (amessage*)mGuestRecvBuffer.mBuffer.data();
            if (kHeaderSize + mesg->data_length ==
                mGuestRecvBuffer.mBufferTail) {
                if (mesg->command == ADB_OPEN) {
                    apacket packet;
                    packet.mesg = *mesg;
                    packet.data.assign(mGuestRecvBuffer.mBuffer.data() + sizeof(amessage),
                                        mGuestRecvBuffer.mBuffer.data() + mGuestRecvBuffer.mBufferTail);
                    mPendingConnections[mesg->arg0] = packet;
                    DD("Open command %s", packet.data.data());
                    //AdbProxy* newProxy = onNewConnection(packet);
                    //if (newProxy) {
                    //    mProxies.emplace(mesg->arg0, newProxy);
                    //}
                } else {
                    mCurrentRecvProxy->second->onGuestRecvData(mGuestRecvBuffer.mBuffer.data(),
                            mGuestRecvBuffer.mBuffer.size());
                    checkCloseProxy(mCurrentRecvProxy);
                    mCurrentRecvProxy = mProxies.end();
                }
                mGuestRecvBuffer.mBufferTail = 0;
            }
        }
    }
    DD("Finish recv buffer");
}

int AdbHub::sendToHost(const AndroidPipeBuffer *buffers,
                       int numBuffers) {
    if (!mSocket) {
        return PIPE_ERROR_IO;
    }
    int result = 0;
    while (numBuffers > 0) {
        const uint8_t* data = buffers[0].data;
        size_t dataSize = buffers[0].size;
        while (dataSize > 0) {
            ssize_t len;
            {
                // Possible that the host socket has been reset.
                if (mSocket && mSocket->valid()) {
                    len = android::base::socketSend(
                            mSocket->fd(), data, dataSize);
                } else {
                    fprintf(stderr, "WARNING: AdbGuestPipe socket closed in the middle of send\n");
                    return PIPE_ERROR_IO;
                }
            }
            if (len > 0) {
                data += len;
                dataSize -= len;
                result += static_cast<int>(len);
                if (dataSize != 0) {
                    return result; // sent less than requested -> no more room.
                } else {
                    break;
                }
            }
            if (len < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                if (result == 0) {
                    //mFdWatcher->dontWantWrite();
                    return PIPE_ERROR_AGAIN;
                }
            } else {
                // End of stream or i/o error means the guest has closed
                // the connection.
                if (result == 0) {
                    //mHostSocket.reset();
                    //mState = State::ClosedByHost;
                    //DINIT("%s: [%p] Adb closed by host",__func__, this);
                    return PIPE_ERROR_IO;
                }
            }
            // Some data was received so report it, the guest will have
            // to try again to get an error.
            return result;
        }
        buffers++;
        numBuffers--;
    }
    return result;
}

void AdbHub::setSocket(CrossSessionSocket *socket) {
    mSocket = socket;
    mSendToHostThread.reset(new base::FunctorThread([this]() {

    }));
}


}  // namespace emulation
}  // namespace android
