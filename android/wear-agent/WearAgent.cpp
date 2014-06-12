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

#include "android/base/Limits.h"
#include "android/wear-agent/PairUpWearPhone.h"
#include "android/wear-agent/WearAgent.h"

extern "C" {
#include "android/async-utils.h"
#include "android/looper.h"
#include "android/sockets.h"
#include "android/utils/debug.h"
#include "android/utils/list.h"
#include "android/utils/misc.h"
}

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define  DPRINT(...)  do {  if (VERBOSE_CHECK(adb)) dprintn(__VA_ARGS__); } while (0)

static void* s_wear_agent = 0;

extern "C" {
void start_wear_agent(Looper* looper) {
    if (!s_wear_agent) {
        s_wear_agent = new android::wear::WearAgent(looper);
    }
}

void stop_wear_agent() {
    if (s_wear_agent) {
        delete ((android::wear::WearAgent*)(s_wear_agent));
        s_wear_agent = 0;
    }
}
}

namespace android {
namespace wear {

class WearAgentImpl {
public:
    WearAgentImpl(Looper*, int);
    ~WearAgentImpl();

    void onWrite();
    void onRead();
    void makeConnection();
private:

    static const int WRITE_BUFFER_SIZE = 256;
    static const int READ_BUFFER_SIZE = 1024;

    int                 mAdbHostPort;
    int                 mSocket;
    Looper*             mLooper;
    LoopIo              mLoopIo[1];
    LoopTimer           mLoopTimer[1];
    int                 mRetryAfterSeconds;
    PairUpWearPhone     mPairUp;
    AsyncReader         mAsyncReader[1];
    AsyncWriter         mAsyncWriter[1];
    char                mWriteBuffer[READ_BUFFER_SIZE];
    char                mReadBuffer[WRITE_BUFFER_SIZE];
    enum EXPCET_MSG {
        OKAY = 0
       ,LENGTH
       ,MESSAGE
    };
    int                 mExpectReplayType;

    bool expectOkay () {return OKAY  ==  mExpectReplayType;}
    bool expectLength() {return LENGTH  ==  mExpectReplayType;}
    bool expectMsg() {return MESSAGE  ==  mExpectReplayType;}
    void cleanupConnection();
    void connectLater(int verbose);
};

static void _talk_to_adb_host(void* opaque, int fd, unsigned events) {
    WearAgentImpl* agent = (WearAgentImpl*)opaque;
    assert(agent);

    if ((events & LOOP_IO_READ) !=  0) {
        agent->onRead();
    }
    if ((events & LOOP_IO_WRITE) !=  0) {
        agent->onWrite();
    }
}

static void _connect_to_adb_host(void* opaque) {
    WearAgentImpl* agent = (WearAgentImpl*)opaque;
    assert(agent);
    agent->makeConnection();
}

WearAgentImpl::WearAgentImpl(Looper* looper, int adbHostPort) : 
        mAdbHostPort(adbHostPort), mSocket(-1), mLooper(looper) ,mLoopIo(),
        mLoopTimer(), mRetryAfterSeconds(2), mPairUp(looper, adbHostPort),
        mAsyncReader(), mAsyncWriter(), mWriteBuffer(), mReadBuffer(),
        mExpectReplayType(OKAY) {
    assert(adbHostPort >0);
    assert(looper);
    loopTimer_init(mLoopTimer, mLooper, _connect_to_adb_host, this);
    static const char kTrackDevicesRequest[] = "host:track-devices";
    snprintf(mWriteBuffer, sizeof(mWriteBuffer), "%04x%s", 
            (unsigned)(::strlen(kTrackDevicesRequest)), kTrackDevicesRequest);
    connectLater(0);
}

WearAgentImpl::~WearAgentImpl() {
    cleanupConnection();
    loopIo_done(mLoopIo);
    loopTimer_done(mLoopTimer);
}


void WearAgentImpl::makeConnection() {
    if (mSocket < 0) {
        mSocket = socket_loopback_client(mAdbHostPort, SOCKET_STREAM);
        if (mSocket >=  0) {
            socket_set_nonblock(mSocket);
        } else {
            cleanupConnection();
            connectLater(1);
            return;
        }
    }
    assert(mSocket >=  0);
    loopIo_init(mLoopIo, mLooper, mSocket, _talk_to_adb_host, this);
    asyncWriter_init(mAsyncWriter, mWriteBuffer, ::strlen(mWriteBuffer), mLoopIo);
}


void WearAgentImpl::connectLater(int verbose) {
    if (verbose) {
        DPRINT("Warning: cannot connect to adb server,"
            "will try again in %d seconds. \n", mRetryAfterSeconds);
    }
    const Duration dl = 1000*mRetryAfterSeconds;
    loopTimer_startRelative(mLoopTimer, dl);
}

void WearAgentImpl::cleanupConnection() {
    if (mSocket >=  0) {
        socket_close(mSocket);
        mSocket = -1;
    }
}

void WearAgentImpl::onWrite() {
    if (mSocket < 0 ) {
        return;
    }
    int status = asyncWriter_write(mAsyncWriter);
    switch (status) {
        case ASYNC_COMPLETE:
            mReadBuffer[4] = '\0';
            asyncReader_init(mAsyncReader, mReadBuffer, 4, mLoopIo);
            mExpectReplayType = OKAY;
            break;
        case ASYNC_ERROR:
            loopIo_dontWantWrite(mLoopIo);
            cleanupConnection();
            connectLater(1);
            break;
        case ASYNC_NEED_MORE:
            return;
    }
}

void WearAgentImpl::onRead() {
    if (mSocket < 0 ) {
        return;
    }
    int msgsize = 0;
    int status = asyncReader_read(mAsyncReader);
    switch (status) {
        case ASYNC_COMPLETE:
            if (expectOkay() && !strncmp("OKAY", mReadBuffer, 4)) {
                mReadBuffer[4] = '\0';
                asyncReader_init(mAsyncReader, mReadBuffer, 4, mLoopIo);
                mExpectReplayType = LENGTH;
            } else if(expectLength() && 1 == sscanf(mReadBuffer, "%x", &msgsize) &&
                    msgsize > 0 && msgsize < READ_BUFFER_SIZE) {
                mReadBuffer[msgsize] = '\0';
                asyncReader_init(mAsyncReader, mReadBuffer, msgsize, mLoopIo);
                mExpectReplayType = MESSAGE;
            } else if (expectMsg()) {
                DPRINT("message received from ADB:\n%s", mReadBuffer);
                loopTimer_stop(mLoopTimer);
                mPairUp.connectWearToPhone(mReadBuffer);
                mReadBuffer[4] = '\0';
                asyncReader_init(mAsyncReader, mReadBuffer, 4, mLoopIo);
                mExpectReplayType = LENGTH;
            } else {
                cleanupConnection();
                connectLater(1);
                return;
            }
            break;
        case ASYNC_NEED_MORE:
            return;
        case ASYNC_ERROR:
            loopIo_dontWantRead(mLoopIo);
            cleanupConnection();
            connectLater(1);
            return;
    }
}

//------------------------- WearAgent Class 
WearAgent::WearAgent(Looper* looper, int adbHostPort) :
        mWearAgentImpl(new WearAgentImpl(looper, adbHostPort)) {
}

WearAgent::~WearAgent() {
    delete mWearAgentImpl;
}

} // end of namespace wear
} // end of namespace android
