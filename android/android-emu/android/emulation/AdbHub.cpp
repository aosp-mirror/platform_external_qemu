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
#include "android/jdwp/JdwpProxy.h"

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
                                  bool skipData,
                                  uint8_t* clonedData, // Hold a copy to read data if not null
                                  size_t* clonedDataSize) {
    bytesToRead = std::min(*remainBytes, bytesToRead);
    while (bytesToRead > 0 && *bufferIdx < numBuffers) {
        size_t remainBufferSize = buffers[*bufferIdx].size - *bufferPst;
        size_t currentReadSize = std::min(bytesToRead, remainBufferSize);
        if (clonedData) {
            memcpy(clonedData + *clonedDataSize, buffers[*bufferIdx].data + *bufferPst, currentReadSize);
        }
        if (clonedDataSize) {
            *clonedDataSize += currentReadSize;
            DD("clonedDataSize %d", (int)*clonedDataSize);
        }
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

AdbHub::~AdbHub() {
    if (mSocket) {
        setSocket(nullptr);
    }
}

void AdbHub::onSave(android::base::Stream *stream) {
    stream->putBe32(mJdwpProxies.size());
    for (const auto& proxy: mJdwpProxies) {
        proxy.second->onSave(stream);
    }
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
    int proxyCount = stream->getBe32();
    for (int i = 0; i < proxyCount; i++) {
        jdwp::JdwpProxy* proxy = new jdwp::JdwpProxy(stream);
        mProxies.emplace(proxy->guestId(), proxy);
        mJdwpProxies.emplace(proxy->mGuestPid, proxy);
    }
    mLoadRecvProxyId = stream->getBe32();
    mLoadSendProxyId = stream->getBe32();
    mCurrentRecvProxy = mProxies.end();
    mCurrentSendProxy = mProxies.end();
    if (mLoadRecvProxyId >= 0) {
        mCurrentRecvProxy = mProxies.find(mLoadRecvProxyId);
        mLoadRecvProxyId = -1;
    }
    if (mLoadSendProxyId >= 0) {
        mCurrentSendProxy = mProxies.find(mLoadSendProxyId);
        mLoadSendProxyId = -1;
    }
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

void AdbHub::tryRemoveProxy(std::unordered_map<int, AdbProxy *>::iterator proxy) {
    if (proxy != mProxies.end() && proxy->second->shouldClose()) {
        mProxies.erase(proxy);
    }
}

int AdbHub::onGuestSendData(const AndroidPipeBuffer* buffers,
                                int numBuffers,
                                OnNewConnection _onNewConnection) {
    const int kMaxBufferSize = 2000000000; // ~2^31 max buffer size
    int actualSentBytesI = 0;
    AndroidPipeBuffer translatedBuffer = {nullptr, 0};
    size_t translatedBufferTail = 0;
    if (mNeedTranslate) {
        for (int i = 0; i < numBuffers; i++) {
            actualSentBytesI += buffers[i].size;
        }
        actualSentBytesI = std::min(actualSentBytesI, kMaxBufferSize);
        translatedBuffer.size = actualSentBytesI;
        translatedBuffer.data = new uint8_t[actualSentBytesI];
        DD("Sending %d buffer %p", actualSentBytesI, translatedBuffer.data);
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
                                        mGuestSendBuffer.mBytesToSkip, true,
                                        translatedBuffer.data, &translatedBufferTail);
        } else if (mGuestSendBuffer.mBufferTail < kHeaderSize) {
            size_t readSize = 0;
            mGuestSendBuffer.readBytes(
                    buffers, numBuffers, &currentBuffer, &currentPst,
                    &bytesToParse,
                    kHeaderSize - mGuestSendBuffer.mBufferTail, false,
                    translatedBuffer.data, &translatedBufferTail);
            if (mGuestSendBuffer.mBufferTail == kHeaderSize) {
                // check header
                amessage* mesg;
                mesg = (amessage*)mGuestSendBuffer.mBuffer.data();
                auto pendingConnection = mPendingConnections.find(mesg->arg1);
                if (mesg->command == ADB_OKAY && pendingConnection != mPendingConnections.end()) {
                    AdbProxy* newProxy = onNewConnection(pendingConnection->second);
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
                    tryRemoveProxy(proxyIte);
                }
            }
        } else {
            size_t readSize = 0;
            amessage* mesg;
            mesg = (amessage*)mGuestSendBuffer.mBuffer.data();
            mGuestSendBuffer.readBytes(buffers, numBuffers, &currentBuffer,
                                       &currentPst, &bytesToParse,
                                       kHeaderSize + mesg->data_length -
                                               mGuestSendBuffer.mBufferTail,
                                       false,
                    translatedBuffer.data, &translatedBufferTail);
            // could have been resized
            mesg = (amessage*)mGuestSendBuffer.mBuffer.data();
            if (kHeaderSize + mesg->data_length == mGuestSendBuffer.mBufferTail) {
                mCurrentSendProxy->second->onGuestSendData(mGuestSendBuffer.mBuffer.data(),
                        mGuestSendBuffer.mBuffer.size());
                tryRemoveProxy(mCurrentSendProxy);
                mCurrentSendProxy = mProxies.end();
                mGuestSendBuffer.mBufferTail = 0;
            }
        }
    }
    if (mNeedTranslate) {
        mSendToHostLock.lock();
        if (!mSocket || !mSocket->valid()) {
            D("Socket invalidated");
            actualSentBytesI = PIPE_ERROR_IO;
        } else {
            DD("Pushed send buffer");
            mSendToHostQueue.push(translatedBuffer);
        }
        mSendToHostCV.signalAndUnlock(&mSendToHostLock);
    }
    return actualSentBytesI;
}

// This function track whether a connection is jdwp or not. It tracks all
// packages until it found a connection package.
int AdbHub::onGuestRecvData(AndroidPipeBuffer* buffers,
                                int numBuffers,
                                OnNewConnection _onNewConnection) {
    using amessage = emulation::AdbMessageSniffer::amessage;
    int actualRecvBytes = recvFromHost(buffers, numBuffers);
    if (actualRecvBytes <= 0) {
        return actualRecvBytes;
    }
    size_t bytesToParse = (size_t)actualRecvBytes;
    const size_t kHeaderSize = sizeof(amessage);
    int readBuffer = 0;
    int currentBuffer = 0;
    size_t currentPst = 0;
    while (currentBuffer < numBuffers && bytesToParse > 0) {
        DD("bytesToParse %d", (int)bytesToParse);
        if (mGuestRecvBuffer.mBytesToSkip) {
            mGuestRecvBuffer.readBytes(buffers, numBuffers, &currentBuffer,
                                       &currentPst, &bytesToParse,
                                       mGuestRecvBuffer.mBytesToSkip, true,
                                       nullptr, nullptr);
        } else if (mGuestRecvBuffer.mBufferTail < kHeaderSize) {
            mGuestRecvBuffer.readBytes(
                    buffers, numBuffers, &currentBuffer, &currentPst,
                    &bytesToParse,
                    kHeaderSize - mGuestRecvBuffer.mBufferTail, false,
                    nullptr, nullptr);
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
                    tryRemoveProxy(proxyIte);
                }
            }
        } else {
            amessage* mesg;
            mesg = (amessage*)mGuestRecvBuffer.mBuffer.data();
            mGuestRecvBuffer.readBytes(buffers, numBuffers, &currentBuffer,
                                       &currentPst, &bytesToParse,
                                       kHeaderSize + mesg->data_length -
                                               mGuestRecvBuffer.mBufferTail,
                                       false, nullptr, nullptr);
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
                    tryRemoveProxy(mCurrentRecvProxy);
                    mCurrentRecvProxy = mProxies.end();
                }
                mGuestRecvBuffer.mBufferTail = 0;
            }
        }
    }
    DD("Finish recv buffer");
    return actualRecvBytes;
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

int AdbHub::recvFromHost(AndroidPipeBuffer* buffers, int numBuffers) {
    if (!mSocket) {
        return PIPE_ERROR_IO;
    }
    int result = 0;
    while (numBuffers > 0) {
        uint8_t* data = buffers[0].data;
        size_t dataSize = buffers[0].size;
        DD("%s: [%p] dataSize=%d", __func__, this, (int)dataSize);
        while (dataSize > 0) {
            ssize_t len;
            {
                // Possible that the host socket has been reset.
                if (mSocket->hasStaleData()) {
                    len = mSocket->readStaleData(data, dataSize);
                    DD("%s: [%p] loaded %d data from buffer", __func__, this,
                       (int)len);
                } else if (mSocket->valid()) {
                    len = android::base::socketRecv(
                            mSocket->fd(), data, dataSize);
                } else {
                    fprintf(stderr, "WARNING: AdbGuestPipe socket closed in the middle of recv\n");
                    len = -1;
                }
            }
            if (len > 0) {
                data += len;
                dataSize -= len;
                result += static_cast<int>(len);
                if (dataSize != 0) {
                    return result; // got less than requested -> no more data.
                } else {
                    break;
                }
            }
            if (len < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                if (result == 0) {
                    //mFdWatcher->dontWantRead();
                    //DD("%s: [%p] done try again", __func__, this);
                    return PIPE_ERROR_AGAIN;
                }
            } else {
                // End of stream or i/o error means the guest has closed
                // the connection.
                if (result == 0) {
                    //mAdbHub.setSocket(nullptr);
                    //mHostSocket.reset();
                    //mState = State::ClosedByHost;
                    //DINIT("%s: [%p] Adb closed by host",__func__, this);
                    return PIPE_ERROR_IO;
                }
            }
            // Some data was received so report it, the guest will have
            // to try again to get an error.
            DD("%s: [%p] done %d", __func__, this, result);
            return result;
        }
        buffers++;
        numBuffers--;
    }
    DD("%s: [%p] done %d", __func__, this, result);
    return result;
}

bool AdbHub::needTranslate() const {
    return mNeedTranslate && mSocket && mSocket->valid();
}

void AdbHub::setSocket(CrossSessionSocket *socket) {
    mSendToHostLock.lock();
    mSocket = nullptr;
    clearSendBufferLocked();
    if (mSendToHostThread) {
        mSendToHostCV.signalAndUnlock(&mSendToHostLock);
        mSendToHostThread->wait();
        mSendToHostLock.lock();
    }
    mSocket = socket;
    // TODO
    mNeedTranslate = mSocket && mSocket->valid();
    mSendToHostLock.unlock();
    if (mNeedTranslate && mSocket && mSocket->valid()) {
        //base::socketSetBlocking(mSocket->fd());
        mSendToHostThread.reset(new base::FunctorThread([this]() {
            bool socketValid = true;
            while (socketValid) {
                mSendToHostLock.lock();
                if (!mSocket || !mSocket->valid()) {
                    break;
                }
                if (mSendToHostQueue.empty()) {
                    mSendToHostCV.wait(&mSendToHostLock);
                    if (!mSocket || !mSocket->valid()) {
                        mSendToHostLock.unlock();
                        break;
                    }
                }
                DD("AdbHub sender thread received data");
                int fd = mSocket->fd();
                AndroidPipeBuffer buffer = mSendToHostQueue.front();
                mSendToHostQueue.pop();
                mSendToHostLock.unlock();
                int length = 0;
                while (length < buffer.size) {
                    int ret = base::socketSend(fd, buffer.data + length, buffer.size - length);
                    if (ret < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // TODO: use the fd watcher
                            sleep(1);
                        } else {
                            DD("AdbHub socket closed during send");
                            socketValid = false;
                            break;
                        }
                    }
                    length += ret;
                }
                delete [] buffer.data;
            }
            D("AdbHub sender thread terminated");
        }));
        mSendToHostThread->start();
    }
}

void AdbHub::clearSendBufferLocked() {
    while (!mSendToHostQueue.empty()) {
        const AndroidPipeBuffer& buffer = mSendToHostQueue.front();
        delete [] buffer.data;
        mSendToHostQueue.pop();
    }
}

AdbProxy* AdbHub::onNewConnection(const apacket &packet) {
    int jdwpId = 0;
    int jdwpConnect = sscanf((const char*)packet.data.data(),
            "jdwp:%d", &jdwpId);
    if (jdwpConnect == 0) {
        return nullptr;
    }
    if (mJdwpProxies.count(jdwpId) == 0) {
        jdwp::JdwpProxy* proxy = new jdwp::JdwpProxy(
            packet.mesg.arg0, packet.mesg.arg1, jdwpId
        );
        mJdwpProxies[jdwpId].reset(proxy);
        return proxy;
    } else if (mAllowReconnect) {
        CHECK(mReconnectingProxy == nullptr);
        mReconnectingProxy = mJdwpProxies[jdwpId].get();
        mReconnectingProxy->mCurrentHostId = packet.mesg.arg0;
        return mReconnectingProxy;
    } else {
        fprintf(stderr, "WARNING: reconnecting to jdwp %d\n", jdwpId);
        return nullptr;
    }
}

int AdbHub::getProxyCount() {
    return mProxies.size();
}

void AdbHub::setAllowReconnect(bool allowReconnect) {
    mAllowReconnect = allowReconnect;
}

}  // namespace emulation
}  // namespace android
