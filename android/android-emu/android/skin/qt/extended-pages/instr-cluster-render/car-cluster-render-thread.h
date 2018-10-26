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

#pragma once
#include <vector>
#include <atomic>

#include <QWidget>
#include <QImage>
#include <QThread>

#include "android/utils/sockets.h"
#include "android/utils/ipaddr.h"

class CarClusterRenderThread : public QThread
{
    Q_OBJECT

public:
    CarClusterRenderThread();

    void stopThread();

signals:
    void renderedImage(const QImage &image);

protected:
    void run();

    // Mock socket operations for testing
    int createSocket();
    int connectSocket();
    int readSocket(char* buf, int buf_size);
    void closeSocket();

private:
    int readPacket(char* buf);
    bool tryConnect();
    bool isFrameStart(std::vector<char> encodedFrames, int ind);
    int findNextFrameIndex(std::vector<char> encodedFrames, int startInd);
    int getNextFrame(char* buf);

    std::atomic<bool> exitRequested = ATOMIC_VAR_INIT(false);

    // TODO: use qemu pipes instead of sockets/tcp
    int mSocket;
    SockAddress mServerAddr;

    std::vector<char> mEncodedData;
    char* mNextFrame;
};
