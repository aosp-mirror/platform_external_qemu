// Copyright 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/skin/qt/extended-pages/instr-cluster-render/car-cluster-render-thread.h"
#include <math.h>
#include <memory>
#include <iostream>

#include "android/base/sockets/SocketUtils.h"
#include "android/base/EintrWrapper.h"

#ifdef _WIN32
#include "android/base/sockets/Winsock.h"
#else
#include <sys/socket.h>
#endif

static constexpr char ADDR[] = "127.0.0.1";
static constexpr int PORT = 1234;
static constexpr int FRAME_WIDTH = 1280;
static constexpr int FRAME_HEIGHT = 720;
static constexpr int BUF_SIZE = 1024;
static constexpr int READ_TIMEOUT_SEC = 2;
static constexpr int CONNECT_TIMEOUT_SEC = 1;
static constexpr int CONNECT_SLEEP_SEC = 1;

CarClusterRenderThread::CarClusterRenderThread() {
    uint32_t ip;
    inet_strtoip(ADDR, &ip);
    sock_address_init_inet(&mServerAddr, ip, PORT);
}

void CarClusterRenderThread::stopThread() {
    exitRequested = true;
}

int CarClusterRenderThread::createSocket() {
    return socket_create_inet(SOCKET_STREAM);
}

int CarClusterRenderThread::readSocket(char* buf, int bufSize) {
    return socket_recv(mSocket, buf, bufSize);
}

int CarClusterRenderThread::connectSocket() {
    return socket_connect(mSocket, &mServerAddr);
}

void CarClusterRenderThread::closeSocket() {
    socket_close(mSocket);
}

void CarClusterRenderThread::run() {
    char* buf = new char[BUF_SIZE];

    int frameSize;
    bool isConnected = false;

    mSocket = createSocket();
    android::base::socketSetNonBlocking(mSocket);

    while (!exitRequested) {
        // Loops until we successfully connect
        if (!isConnected && !(isConnected = tryConnect())) {
            continue;
        }
        frameSize = getNextFrame(buf);
        if (frameSize == -1) {
            // Close current connections, attempt to reconnect on next iteration
            closeSocket();
            isConnected = false;
            mSocket = createSocket();
            continue;
        }

        std::cout << "current frame size: " << frameSize << std::endl;

        // TODO: decode frame to form image & render it
        // just emitting a placeholder for now

        emit renderedImage(QImage());
        delete[] mNextFrame;
    }

    delete[] buf;
    closeSocket();
}

// Returns the size of the packet read, or -1 on error, i.e. broken pipe
int CarClusterRenderThread::readPacket(char* buf) {
    struct timeval tv;
    fd_set fds;

    memset(&tv, 0, sizeof(timeval));
    FD_ZERO(&fds);

    tv.tv_sec = READ_TIMEOUT_SEC;
    FD_SET(mSocket, &fds);

    if (HANDLE_EINTR(::select(mSocket + 1, &fds, 0, 0, &tv)) <= 0) {
        return -1;
    }

    int n = readSocket(buf, BUF_SIZE);

    mEncodedData.insert(mEncodedData.end(), buf, buf + n);
    return n;
}

bool CarClusterRenderThread::tryConnect() {
    struct timeval tv;
    fd_set fds;

    memset(&tv, 0, sizeof(timeval));
    FD_ZERO(&fds);

    tv.tv_sec = CONNECT_TIMEOUT_SEC;
    FD_SET(mSocket, &fds);

    int connRet = connectSocket();
    if (connRet == 0) {
        return true;
    } else if (errno == EWOULDBLOCK ||
                errno == EAGAIN ||
                errno == EINPROGRESS) {
        int selectRet = HANDLE_EINTR(::select(mSocket + 1, 0, &fds, 0, &tv));
        if (selectRet > 0) {
            int err = 0;
            socklen_t optLen = sizeof(err);
            if (getsockopt(mSocket, SOL_SOCKET, SO_ERROR,
                           reinterpret_cast<char*>(&err), &optLen) || err) {
                // error on getsockopt or problem with socket
                this->sleep(CONNECT_SLEEP_SEC);
                return false;
            } else {
                return true;
            }
        } else {
            if (selectRet < 0) {
                // select errored instead of timing out
                this->sleep(CONNECT_SLEEP_SEC);
            }
            return false;
        }
    } else {
        // some other error with connecting
        this->sleep(CONNECT_SLEEP_SEC);
        return false;
    }
}

bool CarClusterRenderThread::isFrameStart(std::vector<char> encodedData, int ind) {
    return (ind + 3) < encodedData.size() && encodedData[ind] == 0 &&
            encodedData[ind + 1] == 0 && encodedData[ind + 2] == 0 && encodedData[ind + 3] == 1;
}

int CarClusterRenderThread::findNextFrameIndex(std::vector<char> encodedData, int startInd) {
    for (int i = startInd; i < encodedData.size(); i++) {
        if (isFrameStart(encodedData, i)) {
            return i;
        }
    }
    return -1;
}

// Returns size of the next frame, or -1 on error, i.e. no frame
// Contents of the next frame are stored in mNextFrame
int CarClusterRenderThread::getNextFrame(char* buf) {
    int packetSize;
    int frameInd = findNextFrameIndex(mEncodedData, 4);
    while (frameInd == -1) {
        packetSize = readPacket(buf);
        if (packetSize == -1) {
            return -1;
        }
        frameInd = findNextFrameIndex(mEncodedData, std::max((std::vector<char>::size_type) 0,
                                                             mEncodedData.size() - packetSize - 3));
    }
    mNextFrame = new char[frameInd];
    copy(mEncodedData.begin(), mEncodedData.begin() + frameInd, mNextFrame);
    mEncodedData.erase(mEncodedData.begin(), mEncodedData.begin() + frameInd);
    return frameInd;
}
