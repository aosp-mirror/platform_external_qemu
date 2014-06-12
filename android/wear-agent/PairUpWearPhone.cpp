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
#include "android/wear-agent/PairUpWearPhone.h"

extern "C" {
#include "android/async-utils.h"
#include "android/looper.h"
#include "android/sockets.h"
#include "android/utils/debug.h"
}

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <queue>
#include <string>
#include <vector>

#define  DPRINT(...)  do {  if (VERBOSE_CHECK(adb)) dprintn(__VA_ARGS__); } while (0)

namespace android {
namespace wear {

class PairUpWearPhoneImpl {
public:
    PairUpWearPhoneImpl(Looper* looper, const char* devicelist, int adbHostPort = 5037);
    ~PairUpWearPhoneImpl();
public:
    void onQueryAdb();
    void onQueryConsole();
    void onAdbReply();
    void onConsoleReply();
    bool done() const;
    void cleanUp();
private:
    Looper*     mLooper;
    int         mAdbHostPort;
    int         mAdbSocket;
    LoopIo      mAdbIo[1];
    int         mConsolePort;
    int         mConsoleSocket;
    LoopIo      mConsoleIo[1];

    typedef std::queue<std::string>  StrQueue;
    typedef std::vector<std::string> StrVec;

    StrQueue    mdevs;
    StrVec      mwears;
    StrQueue    mphones;

    enum COMM_STATE {
         PAIRUP_ERROR = 0
        ,TO_QUERY_PROPERTY_1
        ,TO_READ_PROPERTY_1
        ,TO_QUERY_PROPERTY_2
        ,TO_READ_PROPERTY_2
        ,TO_QUERY_WEARABLE_APP_PKG_1
        ,TO_READ_WEARABLE_APP_PKG_1
        ,TO_QUERY_WEARABLE_APP_PKG_2
        ,TO_READ_WEARABLE_APP_PKG_2
        ,TO_QUERY_FORWARDIP
        ,TO_READ_FORWARDIP
        ,TO_READ_CONSOLE_GREETINGS
        ,TO_QUERY_CONSOLE
        ,PAIRUP_SUCCESS
    };

    COMM_STATE  mState;
    std::string mreply;
    static const int READ_BUFFER_SIZE  = 1024;
    static const int WRITE_BUFFER_SIZE = 1024;

    AsyncReader         mAsyncReader[1];
    AsyncWriter         mAsyncWriter[1];
    char                *mReadBuffer;
    char                *mWriteBuffer;

    // Private methods
    void updateDeviceList(const char* s_devicesupdate_msg);
    void probeNextDevice();

    void openConnection(int port, int& socketfd, LoopIo* io, LoopIoFunc callback);
    void closeConnection(int& socketfd, LoopIo* io);
    void openAdbConnection();
    void closeAdbConnection();
    void openConsoleConnection();
    void closeConsoleConnection();

    enum MSG_TYPE {
         TRANSFER = 1
        ,FORWARDIP
        ,PROPERTY
        ,WEARABLE_APP_PKG
    };
    void switchToReadOnly(LoopIo *io);
    void switchToWriteOnly(LoopIo *io);
    void makeQueryMsg(char buff[], int sz, int type, const char* message);
    void asyncQueryAdb(int queryType, const char* message);
    void parse(const char* msg, StrQueue& tokens);
    void forwardIp();
    int  readAdbOkay();
    bool readAdbReply();
};

// Talks to adb on the host computer
static void _query_adb(void* opaque, int fd, unsigned events) {
    PairUpWearPhoneImpl * pairup = static_cast<PairUpWearPhoneImpl*> (opaque);
    assert(pairup);

    if (events & LOOP_IO_READ) {
        assert((events & LOOP_IO_WRITE) == 0);
        pairup->onAdbReply();
    } else if (events & LOOP_IO_WRITE) {
        assert((events & LOOP_IO_READ) == 0);
        pairup->onQueryAdb();
    }
    if (pairup->done()) {
        pairup->cleanUp();
    }
}

// Talks to emulator console: similar to "telnet localhost console_port"
static void _query_console(void* opaque, int fd, unsigned events) {
    PairUpWearPhoneImpl * pairup = static_cast<PairUpWearPhoneImpl*> (opaque);
    assert(pairup);

    if (events & LOOP_IO_READ) {
        assert((events & LOOP_IO_WRITE) == 0);
        pairup->onConsoleReply();
    } else if (events & LOOP_IO_WRITE) {
        assert((events & LOOP_IO_READ) == 0);
        pairup->onQueryConsole();
    }
    if (pairup->done()) {
        pairup->cleanUp();
    }
}

//-------------- callbacks for IO with Adb  and for IO with Console

void PairUpWearPhoneImpl::onAdbReply() {
    switch(mState) {
        case TO_READ_PROPERTY_1:
            switch (readAdbOkay()) {
                case ASYNC_NEED_MORE:
                    return;
                case ASYNC_ERROR:
                    mState = PAIRUP_ERROR;
                    return;
            }
            asyncQueryAdb(PROPERTY, "ro.build.version.release");

            mState = TO_QUERY_PROPERTY_2;
            return;
        case TO_READ_PROPERTY_2:
            if (!readAdbReply()) {
                return;
            }
            assert(!mdevs.empty());
            if (mreply.find("KKWT") != std::string::npos) {
                DPRINT("found wear %s\n\n", mdevs.front().c_str());
                mwears.push_back(mdevs.front());
                mdevs.pop();
                if (!mwears.empty() && !mphones.empty()) {
                    forwardIp();
                } else {
                    probeNextDevice();
                }
                return;
            }
            closeAdbConnection();
            openAdbConnection();
            asyncQueryAdb(TRANSFER, mdevs.front().c_str());

            mState = TO_QUERY_WEARABLE_APP_PKG_1;
            return;
        case TO_READ_WEARABLE_APP_PKG_1:
            switch (readAdbOkay()) {
                case ASYNC_NEED_MORE:
                    return;
                case ASYNC_ERROR:
                    probeNextDevice();
                    return;
            }
            asyncQueryAdb(WEARABLE_APP_PKG, "wearablepreview");

            mState = TO_QUERY_WEARABLE_APP_PKG_2;
            return;
        case TO_READ_WEARABLE_APP_PKG_2:
            if (!readAdbReply()) {
                return;
            }
            assert(!mdevs.empty());
            if (mreply.find("wearablepreview") !=  std::string::npos) {
                DPRINT("found phone %s\n\n", mdevs.front().c_str());
                mphones.push(mdevs.front());
                mdevs.pop();
                if (!mwears.empty() && !mphones.empty()) {
                    forwardIp();
                    return;
                }
            } else {
                if (mreply.find("Error:") !=  std::string::npos &&
                        mreply.find("Is the system running?") !=  std::string::npos) {
                    // try again later: package manager is not up and running yet
                    std::string current = mdevs.front();
                    mdevs.push(current);
                }
                mdevs.pop();
            }
            probeNextDevice();
            return;
        case TO_READ_FORWARDIP:
            switch (readAdbOkay()) {
                case ASYNC_NEED_MORE:
                    return;
                case ASYNC_ERROR:
                    probeNextDevice();
                    return;
            }
            mState = PAIRUP_SUCCESS;
            return;
        default:

            mState = PAIRUP_ERROR;
            return;
    }
}

void PairUpWearPhoneImpl::onQueryAdb() {
    assert(mAdbSocket >= 0 && mReadBuffer);

    int status = asyncWriter_write(mAsyncWriter);
    if (ASYNC_ERROR == status) {
        DPRINT("Error: cannot setup port transfer\n");
        mState = PAIRUP_ERROR;
        probeNextDevice();
        return;
    } else if (ASYNC_NEED_MORE == status) {
        return;
    }

    switch (mState) {
        case TO_QUERY_PROPERTY_1:
            mReadBuffer[4]='\0';
            asyncReader_init(mAsyncReader, mReadBuffer, 4, mAdbIo);
            mState = TO_READ_PROPERTY_1;
            break;
        case TO_QUERY_PROPERTY_2:
            mState = TO_READ_PROPERTY_2;
            break;
        case TO_QUERY_WEARABLE_APP_PKG_1:
            mReadBuffer[4]='\0';
            asyncReader_init(mAsyncReader, mReadBuffer, 4, mAdbIo);
            mState = TO_READ_WEARABLE_APP_PKG_1;
            break;
        case TO_QUERY_WEARABLE_APP_PKG_2:
            mState = TO_READ_WEARABLE_APP_PKG_2;
            break;
        case TO_QUERY_FORWARDIP:
            mReadBuffer[4]='\0';
            asyncReader_init(mAsyncReader, mReadBuffer, 4, mAdbIo);
            mState = TO_READ_FORWARDIP;
            break;
        default:
            break;
    }
    switchToReadOnly(mAdbIo);
}

void PairUpWearPhoneImpl::onConsoleReply() {
    assert(mConsoleSocket >= 0 && mReadBuffer);
    const int size = socket_recv(mConsoleSocket, mReadBuffer, READ_BUFFER_SIZE);
    if (size <= 0) {
        probeNextDevice();
        return;
    }
    mreply.append(mReadBuffer, size);
    if (mreply.find("OK") != std::string::npos) {
        DPRINT("received mesg from console:\n%s\n", mreply.c_str());
        snprintf(mWriteBuffer, WRITE_BUFFER_SIZE, "redir add tcp:5601:5601\nquit\n");
        DPRINT("sending query to console:\n%s", mWriteBuffer);
        asyncWriter_init(mAsyncWriter, mWriteBuffer, ::strlen(mWriteBuffer), mConsoleIo);
        switchToWriteOnly(mConsoleIo);
        mState = TO_QUERY_CONSOLE;
    }
}

void PairUpWearPhoneImpl::onQueryConsole() {
    assert(mConsoleSocket >= 0);
    int status = asyncWriter_write(mAsyncWriter);
    if (ASYNC_COMPLETE == status) {
        mState = PAIRUP_SUCCESS;
    } else if (ASYNC_ERROR == status) {
        DPRINT("Error: cannot write to console\n");
        probeNextDevice();
    }
}

//------------------ end of callbacks related to IO

void PairUpWearPhoneImpl::asyncQueryAdb(int queryType, const char* message) {
    makeQueryMsg(mWriteBuffer, WRITE_BUFFER_SIZE, queryType, message);
    DPRINT("sending query '%s' to adb\n", mWriteBuffer);
    asyncWriter_init(mAsyncWriter, mWriteBuffer, ::strlen(mWriteBuffer), mAdbIo);
    switchToWriteOnly(mAdbIo);
}

void PairUpWearPhoneImpl::switchToWriteOnly(LoopIo *io) {
    loopIo_dontWantRead(io);
    loopIo_wantWrite(io);
}

void PairUpWearPhoneImpl::switchToReadOnly(LoopIo *io) {
    mreply.clear();
    loopIo_dontWantWrite(io);
    loopIo_wantRead(io);
}

PairUpWearPhoneImpl::PairUpWearPhoneImpl(Looper* looper, const char* devicelist,
        int adbHostPort) :
        mLooper(looper), mAdbHostPort(adbHostPort), mAdbSocket(-1),
        mAdbIo(), mConsolePort(-1), mConsoleSocket(-1), mConsoleIo(),
        mdevs(), mwears(), mphones(), mState(PAIRUP_ERROR),
        mAsyncReader(), mAsyncWriter(),
        mReadBuffer(static_cast<char*>(calloc(READ_BUFFER_SIZE,1))),
        mWriteBuffer(static_cast<char*>(calloc(WRITE_BUFFER_SIZE,1))) {
    assert(looper);
    assert(adbHostPort > 1024);
    updateDeviceList(devicelist);
}

PairUpWearPhoneImpl::~PairUpWearPhoneImpl() {
    cleanUp();
    mLooper = 0;
    mAdbHostPort = -1;
    mConsolePort = -1;
}

void PairUpWearPhoneImpl::probeNextDevice() {
    closeAdbConnection();
    closeConsoleConnection();
    if (!mdevs.empty()) {
        DPRINT("continue to probe ...\n\n");
        openAdbConnection();
        if (mAdbSocket < 0) {
            mState = PAIRUP_ERROR;
        } else {
            asyncQueryAdb(TRANSFER, mdevs.front().c_str());
            mState = TO_QUERY_PROPERTY_1;
        }
    } else {
        DPRINT("there is no more device to probe.\n\n");
        mState = PAIRUP_ERROR;
    }
}

int PairUpWearPhoneImpl::readAdbOkay() {
    int status = asyncReader_read(mAsyncReader);
    if (ASYNC_NEED_MORE == status) {
        return status;
    } else if (ASYNC_COMPLETE == status && !strncmp("OKAY", mReadBuffer,4)) {
        DPRINT("received mesg from adb host:%s\n", mReadBuffer);
        return ASYNC_COMPLETE;
    } else {
        return ASYNC_ERROR;
    }
}

bool PairUpWearPhoneImpl::readAdbReply() {
    assert(mAdbSocket >= 0);
    int size = 0;
    char buff[1024];
    size = socket_recv(mAdbSocket, buff, sizeof(buff));
    if (size < 0) {
        return false;
    } else if (size > 0) {
        mreply.append(buff, size);
        return false;
    }
    DPRINT("received mesg from adb host:%s\n", mreply.c_str());
    return true;
}

void PairUpWearPhoneImpl::cleanUp() {
    closeAdbConnection();
    closeConsoleConnection();
    {
        StrQueue q1, q2;
        std::swap(mdevs,q1);
        std::swap(mphones,q2);
    }
    assert(mdevs.empty());
    assert(mphones.empty());
    mwears.clear();
    mreply.clear();
    if (mReadBuffer) {
        free(mReadBuffer);
        mReadBuffer = 0;
    }
    if (mWriteBuffer) {
        free(mWriteBuffer);
        mWriteBuffer = 0;
    }
    DPRINT("\nclean up\n\n");
}

bool PairUpWearPhoneImpl::done() const {
    return (PAIRUP_ERROR == mState || PAIRUP_SUCCESS == mState);
}

void PairUpWearPhoneImpl::parse(const char* msg, StrQueue& tokens)
{
    const char* kDelimiters = " \t\r\n";
    char* buf = strdup(msg);
    char* pch = strtok(buf, kDelimiters);
    char* prev = NULL;
    while (pch) {
        if (!strcmp("device", pch) && prev) {
                tokens.push(prev);
        }
        prev = pch;
        pch = strtok(NULL, kDelimiters);
    }

    free(buf);
}

void PairUpWearPhoneImpl::updateDeviceList(const char* s_devicesupdate_msg) {
    DPRINT("start paring up wear to phone\n");
    StrQueue devices;
    parse(s_devicesupdate_msg, devices);
    if (devices.size() < 2) {
        DPRINT("Error: cannot handle device list: %s\n", s_devicesupdate_msg);
        mState = PAIRUP_ERROR;
        return;
    }
    std::swap(mdevs,devices);
    probeNextDevice();
}

void PairUpWearPhoneImpl::forwardIp() {
    closeAdbConnection();
    closeConsoleConnection();
    assert(!mwears.empty() && !mphones.empty());
    const char* phone = mphones.front().c_str();
    const char* emu = "emulator-";
    const int sz = ::strlen(emu);
    if (strncmp(emu, phone, sz)) {
        // usb phone
        // "host-serial:phone-serial-number:forward:tcp:5601;5601"
        openAdbConnection();
        if (mAdbSocket < 0) {
            mState = PAIRUP_ERROR;
        } else {
            asyncQueryAdb(FORWARDIP, phone);
            mState = TO_QUERY_FORWARDIP;
        }
    } else {
        // emulated phone
        // the host-serial:emulator-serial-number:forward command
        // does not work, so we need to work around and connect
        // to the emulator console and issue command "redir add"
        mConsolePort = atoi(phone + sz);
        assert(mConsolePort >= 5554);
        assert (mConsoleSocket < 0);
        openConsoleConnection();
        if (mConsoleSocket < 0) {
            probeNextDevice();
        } else {
            switchToReadOnly(mConsoleIo);
            mState = TO_READ_CONSOLE_GREETINGS;
        }
    }

    mphones.pop();
}

//-----------------------            helper methods

void PairUpWearPhoneImpl::openAdbConnection() {
    openConnection(mAdbHostPort, mAdbSocket, mAdbIo, _query_adb);
}

void PairUpWearPhoneImpl::openConsoleConnection() {
    openConnection(mConsolePort, mConsoleSocket, mConsoleIo, _query_console);
}

void PairUpWearPhoneImpl::closeAdbConnection() {
    closeConnection(mAdbSocket, mAdbIo);
}

void PairUpWearPhoneImpl::closeConsoleConnection() {
    closeConnection(mConsoleSocket, mConsoleIo);
}

void PairUpWearPhoneImpl::closeConnection(int& so, LoopIo* io) {
    if (so >= 0) {
        socket_close(so);
        so = -1;
        loopIo_dontWantRead(io);
        loopIo_dontWantWrite(io);
        loopIo_done(io);
    }
}

void PairUpWearPhoneImpl::openConnection(int port, int& so, LoopIo* io, LoopIoFunc callback) {
    assert(so < 0);
    so = socket_loopback_client(port, SOCKET_STREAM);
    if (so >= 0) {
        socket_set_nonblock(so);
        loopIo_init(io, mLooper, so, callback, this);
        DPRINT("successfully opened up connection to port %d\n", port);
    } else {
        DPRINT("failed to open up connection to port %d\n", port);
    }
}

void PairUpWearPhoneImpl::makeQueryMsg(char *buff, int sz, int type, const char* message) {
    assert(message);
    char buf2[1024];
    switch(type) {
        case TRANSFER:
            snprintf(buf2, sizeof(buf2), "host:transport:%s", message);
            break;
        case PROPERTY:
            snprintf(buf2, sizeof(buf2), "shell:getprop %s", message);
            break;
        case WEARABLE_APP_PKG:
            snprintf(buf2, sizeof(buf2), "shell:pm list packages %s", message);
            break;
        case FORWARDIP:
            snprintf(buf2, sizeof(buf2), "host-serial:%s:forward:tcp:5601;tcp:5601", message);
            break;
        default:
            buf2[0] = '\0';
            break;
    }
    snprintf(buff, sz, "%04x%s", (unsigned)(::strlen(buf2)), buf2);
    assert(::strlen(buff) < (int)WRITE_BUFFER_SIZE);
}

// ------------------------------------------------------------------
//
//       PairUpWearPhone  implementation
//
// ------------------------------------------------------------------

PairUpWearPhone::PairUpWearPhone(Looper* looper, const char* devicelist, int adbHostPort) :
    mPairUpWearPhoneImpl (new PairUpWearPhoneImpl(looper, devicelist, adbHostPort)) {
}

PairUpWearPhone::~PairUpWearPhone() {
    delete mPairUpWearPhoneImpl;
}

} // namespace wear
} // namespace android
