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

#include <android/base/Limits.h>
#include "android/async-utils.h"
#include "android/looper.h"
#include "android/sockets.h"
#include "android/utils/debug.h"
#include "android/wear-agent/PairUpWearPhone.h"
#include "android/wear-agent/WearAgent.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>

#define  DPRINT(...)  do {  if (VERBOSE_CHECK(adb)) dprintn(__VA_ARGS__); } while (0)

static android::wear::WearAgent *s_wear_agent = 0;

extern "C" {
void wear_agent_start(Looper* looper) {
    if (!s_wear_agent) {
        s_wear_agent = new android::wear::WearAgent(looper);
    }
}

void wear_agent_stop() {
    if (s_wear_agent) {
        delete s_wear_agent;
        s_wear_agent = 0;
    }
}

} // extern "C"

namespace android {
namespace wear {

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
    Looper*      mLooper;
    LoopIo       mLoopIo[1];
    LoopTimer    mLoopTimer[1];
    int          mRetryAfterSeconds;
    AsyncReader  mAsyncReader[1];
    AsyncWriter  mAsyncWriter[1];
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
    void parseAdbDevices(char* buf, std::vector<std::string> & devices);
};

static void _on_adb_server_socket_fd(void* opaque, int fd, unsigned events) {
    WearAgentImpl* agent = (WearAgentImpl*)opaque;
    if (!agent) return;

    if ((events & LOOP_IO_READ) != 0) {
        agent->onRead();
    }else if ((events & LOOP_IO_WRITE) != 0) {
        agent->onWrite();
    }
}

void WearAgentImpl::onRead() {
    if (mSocket < 0) return;

    bool isError = false;
    int msgsize = 0;
    int status = asyncReader_read(mAsyncReader);

    switch (status) {
        case ASYNC_COMPLETE:
            if (expectOkay() && !strncmp("OKAY", mReadBuffer, 4)) {
                loopTimer_stop(mLoopTimer);
                mReadBuffer[4] = '\0';
                asyncReader_init(mAsyncReader, mReadBuffer, 4, mLoopIo);
                mExpectReplayType = LENGTH;

            } else if (expectLength() && isValidHexNumber(mReadBuffer, 4)
                    && 1 == sscanf(mReadBuffer, "%x", &msgsize)) {
                if (msgsize < 0) {
                    isError = true;
                } else if (0 == msgsize) { //this is not error: just no devices
                    mExpectReplayType = LENGTH;
                    mReadBuffer[msgsize] = '\0';
                    asyncReader_init(mAsyncReader, mReadBuffer, 4, mLoopIo);
                } else {
                    if (msgsize >= mSizeOfReadBuffer) {
                        char* ptr = (char*)calloc(2*msgsize, sizeof(char));
                        if (ptr) {
                            mSizeOfReadBuffer = 2*msgsize;
                            free(mReadBuffer);
                            mReadBuffer = ptr;
                        }
                    }
                    mExpectReplayType = MESSAGE;
                    mReadBuffer[msgsize] = '\0';
                    asyncReader_init(mAsyncReader, mReadBuffer, msgsize, mLoopIo);
                }

            } else if (expectMsg()) {
                DPRINT("message received from ADB:\n%s", mReadBuffer);
                if (mPairUpWearPhone) {
                    delete mPairUpWearPhone;
                    mPairUpWearPhone = 0;
                }
                std::vector<std::string> devices;
                parseAdbDevices(mReadBuffer, devices);
                if (devices.size() >= 2) {
                    mPairUpWearPhone = new PairUpWearPhone(mLooper, devices, mAdbHostPort);
                }
                // prepare for next adb devices message
                mReadBuffer[4] = '\0';
                asyncReader_init(mAsyncReader, mReadBuffer, 4, mLoopIo);
                mExpectReplayType = LENGTH;
            } else {
                isError = true;
            }
            break;

        case ASYNC_NEED_MORE:
            return;

        case ASYNC_ERROR:
            isError = true;
            break;
    }

    if (isError) {
        cleanupConnection();
        connectLater();
    }
}

void WearAgentImpl::parseAdbDevices(char* buf, std::vector<std::string> & devices) {
    if (!buf) return;

    static const char* kDelimiters = " \t\r\n";

    char* pch = strtok(buf, kDelimiters);
    char* prev = NULL;
    while (pch) {
        if (!strcmp("device", pch) && prev) {
            devices.push_back(prev);
        }
        prev = pch;
        pch = strtok(NULL, kDelimiters);
    }
}

void WearAgentImpl::onWrite() {
    if (mSocket < 0) return;

    int status = asyncWriter_write(mAsyncWriter);

    switch (status) {
        case ASYNC_COMPLETE:
            mReadBuffer[4] = '\0';
            asyncReader_init(mAsyncReader, mReadBuffer, 4, mLoopIo);
            mExpectReplayType = OKAY;
            return;
        case ASYNC_ERROR:
            cleanupConnection();
            connectLater();
            return;
        case ASYNC_NEED_MORE:
            return;
    }
}

static void _on_time_up(void* opaque) {
    WearAgentImpl* agent = (WearAgentImpl*)opaque;
    if (!agent) return;

    agent->connectToAdbHost();
}

void WearAgentImpl::connectToAdbHost() {
    if (mSocket < 0) {
        mSocket = socket_loopback_client(mAdbHostPort, SOCKET_STREAM);
        if (mSocket >= 0) {
            socket_set_nonblock(mSocket);
            loopIo_init(mLoopIo, mLooper, mSocket, _on_adb_server_socket_fd, this);
            asyncWriter_init(mAsyncWriter, mWriteBuffer, ::strlen(mWriteBuffer), mLoopIo);
        } else {
            connectLater();
        }
    }
}

void WearAgentImpl::connectLater() {
    DPRINT("Warning: cannot connect to adb server, will try again in %d seconds. \n",
            mRetryAfterSeconds);
    const Duration dl = 1000*mRetryAfterSeconds;
    loopTimer_startRelative(mLoopTimer, dl);
}

void WearAgentImpl::cleanupConnection() {
    if (mSocket >= 0) {
        loopIo_dontWantRead(mLoopIo);
        loopIo_dontWantWrite(mLoopIo);
        socket_close(mSocket);
        loopIo_done(mLoopIo);
        mSocket = -1;
    }

    if (mPairUpWearPhone) {
        delete mPairUpWearPhone;
        mPairUpWearPhone = 0;
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
        mAdbHostPort(adbHostPort), mSocket(-1), mLooper(looper) ,mLoopIo(),
        mLoopTimer(), mRetryAfterSeconds(2), mAsyncReader(), mAsyncWriter(),
        mSizeOfReadBuffer(DEFAULT_READ_BUFFER_SIZE), mReadBuffer(0),
        mWriteBuffer(), mExpectReplayType(OKAY), mPairUpWearPhone(0) {

    mReadBuffer = (char*)calloc(mSizeOfReadBuffer,sizeof(char));
    loopTimer_init(mLoopTimer, mLooper, _on_time_up, this);

    static const char kTrackDevicesRequest[] = "host:track-devices";

    snprintf(mWriteBuffer, sizeof(mWriteBuffer), "%04x%s",
            (unsigned)(::strlen(kTrackDevicesRequest)), kTrackDevicesRequest);
    connectToAdbHost();
}

WearAgentImpl::~WearAgentImpl() {
    cleanupConnection();
    loopTimer_done(mLoopTimer);
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
