// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/wear-agent/WearAgent.h"

#include "android/base/Limits.h"
#include "android/base/Log.h"
#include "android/base/async/AsyncReader.h"
#include "android/base/async/AsyncWriter.h"
#include "android/base/async/Looper.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/String.h"
#include "android/base/containers/StringVector.h"
#include "android/wear-agent/PairUpWearPhone.h"
#include "android/wear-agent/WearAgent.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DPRINT(...)  ((void)0)

//#define  DPRINT(...)  do {  if (VERBOSE_CHECK(adb)) dprintn(__VA_ARGS__); } while (0)

namespace android {
namespace wear {

using namespace ::android::base;

/*
 * Wear Agent listens to adb host server for device list update.
 * Whenever it receives an update, it will abort current pairing process
 * and start a new one. When adb host dies, Wear Agent will try to reconnect
 * every two seconds.
 */

class WearAgentImpl {
public:
    WearAgentImpl(Looper* looper, int adbHostPort);
    ~WearAgentImpl();

    void onWrite();
    void onRead();
    void connectToAdbHost();
private:
    static const int DEFAULT_READ_BUFFER_SIZE = 1024;
    static const int WRITE_BUFFER_SIZE = 1024;
    int          mAdbHostPort;
    int          mSocket;
    Looper* mLooper;
    ScopedPtr<Looper::FdWatch> mFdWatch;
    ScopedPtr<Looper::Timer> mTimer;
    int          mRetryAfterSeconds;
    ::android::base::AsyncReader  mAsyncReader;
    ::android::base::AsyncWriter  mAsyncWriter;
    int          mSizeOfReadBuffer;
    char         *mReadBuffer;
    char         mWriteBuffer[WRITE_BUFFER_SIZE];
    int          mExpectReplayType;
    PairUpWearPhone   *mPairUpWearPhone;

    enum ExpectMessage {
        OKAY = 0,
        LENGTH,
        MESSAGE
    };

    bool expectOkay () const { return OKAY == mExpectReplayType; }
    bool expectLength() const { return LENGTH == mExpectReplayType; }
    bool expectMsg() const { return MESSAGE == mExpectReplayType; }
    void cleanupConnection();
    void connectLater();
    bool isValidHexNumber(const char* str, const int sz);
    void parseAdbDevices(char* buf, StringVector* devices);
};

// This callback is called whenever an I/O event happens on the socket
// connecting the agent to the host ADB server.
void _on_adb_server_socket_fd(void* opaque, int fd, unsigned events) {
    WearAgentImpl* agent = reinterpret_cast<WearAgentImpl*>(opaque);
    if (!agent) {
        return;
    }
    if ((events & Looper::FdWatch::kEventRead) != 0) {
        agent->onRead();
    } else if ((events & Looper::FdWatch::kEventWrite) != 0) {
        agent->onWrite();
    }
}

void WearAgentImpl::onRead() {
    if (mSocket < 0) {
        return;
    }

    bool isError = false;
    int msgsize = 0;
    AsyncStatus status = mAsyncReader.run();

    switch (status) {
        case kAsyncCompleted:
            if (expectOkay() && !strncmp("OKAY", mReadBuffer, 4)) {
                mTimer->stop();
                mReadBuffer[4] = '\0';
                mAsyncReader.reset(mReadBuffer, 4, mFdWatch.get());
                mExpectReplayType = LENGTH;

            } else if (expectLength() && isValidHexNumber(mReadBuffer, 4)
                    && 1 == sscanf(mReadBuffer, "%x", &msgsize)) {
                if (msgsize < 0) {
                    isError = true;
                } else if (0 == msgsize) { //this is not error: just no devices
                    mExpectReplayType = LENGTH;
                    mReadBuffer[msgsize] = '\0';
                    mAsyncReader.reset(mReadBuffer, 4, mFdWatch.get());
                } else {
                    if (msgsize >= mSizeOfReadBuffer) {
                        char* ptr = (char*)calloc(2 * msgsize, sizeof(char));
                        if (ptr) {
                            mSizeOfReadBuffer = 2 * msgsize;
                            free(mReadBuffer);
                            mReadBuffer = ptr;
                        }
                    }
                    mExpectReplayType = MESSAGE;
                    mReadBuffer[msgsize] = '\0';
                    mAsyncReader.reset(mReadBuffer, msgsize, mFdWatch.get());
                }

            } else if (expectMsg()) {
                DPRINT("message received from ADB:\n%s", mReadBuffer);
                if (mPairUpWearPhone) {
                    delete mPairUpWearPhone;
                    mPairUpWearPhone = 0;
                }
                StringVector devices;
                parseAdbDevices(mReadBuffer, &devices);
                if (devices.size() >= 2) {
                    mPairUpWearPhone = new PairUpWearPhone(mLooper,
                                                           devices,
                                                           mAdbHostPort);
                }
                // prepare for next adb devices message
                mReadBuffer[4] = '\0';
                mAsyncReader.reset(mReadBuffer, 4, mFdWatch.get());
                mExpectReplayType = LENGTH;
            } else {
                isError = true;
            }
            break;

        case kAsyncAgain:
            return;

        case kAsyncError:
            isError = true;
            break;
    }

    if (isError) {
        cleanupConnection();
        connectLater();
    }
}

void WearAgentImpl::onWrite() {
    if (mSocket < 0) {
        return;
    }

    AsyncStatus status = mAsyncWriter.run();
    switch (status) {
        case kAsyncCompleted:
            mReadBuffer[4] = '\0';
            mAsyncReader.reset(mReadBuffer, 4, mFdWatch.get());
            mExpectReplayType = OKAY;
            return;
        case kAsyncError:
            cleanupConnection();
            connectLater();
            return;
        case kAsyncAgain:
            return;
    }
}

void WearAgentImpl::connectToAdbHost() {
    if (mSocket < 0) {
        mSocket = socketTcpLoopbackClient(mAdbHostPort);
        if (mSocket >= 0) {
            socketSetNonBlocking(mSocket);
            mFdWatch.reset(mLooper->createFdWatch(mSocket,
                                                  _on_adb_server_socket_fd,
                                                  this));
            mAsyncWriter.reset(mWriteBuffer,
                               ::strlen(mWriteBuffer),
                               mFdWatch.get());
        } else {
            connectLater();
        }
    }
}

void _on_reconnect_timeout(void* opaque) {
    WearAgentImpl* agent = reinterpret_cast<WearAgentImpl*>(opaque);
    if (agent) {
        agent->connectToAdbHost();
    }
}

void WearAgentImpl::connectLater() {
    DPRINT("Warning: cannot connect to adb server, will try again in %d seconds. \n",
            mRetryAfterSeconds);
    const Looper::Duration dl = 1000 * mRetryAfterSeconds;
    mTimer->startRelative(dl);
}

void WearAgentImpl::cleanupConnection() {
    if (mSocket >= 0) {
        delete mFdWatch.release();
        socketClose(mSocket);
        mSocket = -1;
    }

    if (mPairUpWearPhone) {
        delete mPairUpWearPhone;
        mPairUpWearPhone = 0;
    }
}

void WearAgentImpl::parseAdbDevices(char* buf, StringVector* devices) {
    if (!buf) return;

    static const char kDelimiters[] = " \t\r\n";

    char* pch = strtok(buf, kDelimiters);
    char* prev = NULL;
    while (pch) {
        if (!strcmp("device", pch) && prev) {
            devices->push_back(String(prev));
        }
        prev = pch;
        pch = strtok(NULL, kDelimiters);
    }
}

bool WearAgentImpl::isValidHexNumber(const char* str, const int sz) {
    if (!str || sz <= 0)
        return false;
    for (int i=0; i < sz; ++i) {
        int ch = tolower(str[i]);
        if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f')) {
            continue;
        } else {
            return false;
        }
    }
    return true;
}

WearAgentImpl::WearAgentImpl(Looper* looper, int adbHostPort) :
        mAdbHostPort(adbHostPort),
        mSocket(-1),
        mLooper(looper),
        mFdWatch(),
        mTimer(),
        mRetryAfterSeconds(2),
        mAsyncReader(),
        mAsyncWriter(),
        mSizeOfReadBuffer(DEFAULT_READ_BUFFER_SIZE),
        mReadBuffer(0),
        mWriteBuffer(),
        mExpectReplayType(OKAY),
        mPairUpWearPhone(0) {

    mReadBuffer = (char*)calloc(mSizeOfReadBuffer,sizeof(char));
    mTimer.reset(looper->createTimer(_on_reconnect_timeout, this));

    static const char kTrackDevicesRequest[] = "host:track-devices";

    snprintf(mWriteBuffer, sizeof(mWriteBuffer), "%04x%s",
             static_cast<unsigned>(sizeof(kTrackDevicesRequest) - 1U),
             kTrackDevicesRequest);

    connectToAdbHost();
}

WearAgentImpl::~WearAgentImpl() {
    cleanupConnection();
    free(mReadBuffer);
}

//------------------------- WearAgent Class
WearAgent::WearAgent(Looper* looper, int adbHostPort) : mWearAgentImpl(0) {
    if (looper && adbHostPort >= 5037) {
        mWearAgentImpl = new WearAgentImpl(looper, adbHostPort);
    }
}

WearAgent::~WearAgent() {
    if (mWearAgentImpl) {
        delete mWearAgentImpl;
        mWearAgentImpl = 0;
    }
}

} // namespace wear
} // namespace android
