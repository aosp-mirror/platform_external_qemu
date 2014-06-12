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
#include <android/base/String.h>
#include "android/wear-agent/PairUpWearPhone.h"

extern "C" {
#include "android/android.h"
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

namespace android {
namespace wear {

class PairUpWearPhoneImpl {
public:
    PairUpWearPhoneImpl(Looper* looper, int adbHostPort = 5037);
    ~PairUpWearPhoneImpl();
public:
    void connectWearToPhone(const char* s_devicesupdate_msg);
    void onQueryAdb();
    void onQueryConsole();
    void onAdbReply();
    void onConsoleReply();
private:

    Looper*     mLooper;
    int         mAdbHostPort;
    int         mAdbSocket;
    LoopIo      mAdbIo[1];
    int         mConsolePort;
    int         mConsoleSocket;
    LoopIo      mConsoleIo[1];

    typedef std::vector<std::string> StrVec;

    StrVec      mdevs;
    StrVec      mwears;
    StrVec      mphones;

    enum COMM_STATE {
         PAIRUP_ERROR = 0
        ,TO_QUERY_XFER
        ,TO_READ_XFER_OKAY
        ,TO_QUERY_PROPERTY
        ,TO_READ_PROPERTY
        ,TO_QUERY_WEARABLE_APP_PKG_1
        ,TO_READ_WEARABLE_APP_PKG_1
        ,TO_QUERY_WEARABLE_APP_PKG_2
        ,TO_READ_WEARABLE_APP_PKG_2
        ,TO_QUERY_FORWARDIP
        ,TO_READ_FORWARDIP_OKAY
        ,QUERY_DONE
        ,FORWARDIP_DONE
    };

    COMM_STATE  mstate;
    std::string mreply;
    std::string mdevicesmsg;
    int         mrestart;
    static const int WRITE_BUFFER_SIZE = 256;
    static const int READ_BUFFER_SIZE = 1024;

    AsyncReader         mAsyncReader[1];
    AsyncWriter         mAsyncWriter[1];
    char                mReadBuffer[WRITE_BUFFER_SIZE];
    char                mWriteBuffer[READ_BUFFER_SIZE];

private:
    void openConnection(int port, int& socketfd, LoopIo* io);
    void closeConnection(int& socketfd, LoopIo* io);
    void openAdbConnection();
    void closeAdbConnection();
    void openConsoleConnection();
    void closeConsoleConnection();
    void checkQueryCompletion();

    enum MSG_TYPE {
         TRANSFER = 1
        ,FORWARDIP
        ,PROPERTY
        ,WEARABLE_APP_PKG
    };
    void makeQueryMsg(char buff[], int sz, const char* serial, MSG_TYPE type, 
            const char* property = NULL);
    void parse(const char* msg, StrVec& tokens);

    void forwardIp(const char* phone_serial_number);

    void restart();
};


//talks to adb on the host computer
static void _query_adb(void* opaque, int fd, unsigned events) {
    PairUpWearPhoneImpl * pairup = static_cast<PairUpWearPhoneImpl*> (opaque);
    assert(pairup);

    if ((events & LOOP_IO_READ) !=  0) {
        pairup->onAdbReply();
    }
    if ((events & LOOP_IO_WRITE) !=  0) {
        pairup->onQueryAdb();
    }
}

//talks to emulator console
//similar to "telnet localhost console_port"
static void _query_console(void* opaque, int fd, unsigned events) {
    PairUpWearPhoneImpl * pairup = static_cast<PairUpWearPhoneImpl*> (opaque);
    assert(pairup);

    if ((events & LOOP_IO_READ) !=  0) {
        pairup->onConsoleReply();
    }
    if ((events & LOOP_IO_WRITE) !=  0) {
        pairup->onQueryConsole();
    }
}



//-------------- callbacks for IO with Adb  and for IO with Console
void PairUpWearPhoneImpl::onAdbReply() {
    if (mrestart) {
        restart();
        return;
    }

    if (mAdbSocket < 0 || !mAdbIo) {
        return;
    }
    
    if(mdevs.empty() && mstate != TO_READ_FORWARDIP_OKAY) {
        return;
    }

    int status=0;
    int size = 0;
    char buff[4096] = {'\0'};
    buff[0] = '\0';
    char* pch = buff;
    switch(mstate) {
        case TO_READ_XFER_OKAY:
            status = asyncReader_read(mAsyncReader);
            if (ASYNC_NEED_MORE == status) {
                return;
            } else if (ASYNC_ERROR == status || strncmp("OKAY", mReadBuffer,4)) {
                DPRINT("Error: cannot get OKAY response from ADB server\n");
                mstate = PAIRUP_ERROR;
                return;
            }
            makeQueryMsg(mWriteBuffer, sizeof(mWriteBuffer), mdevs.back().c_str(), PROPERTY,
                    "ro.build.version.release");
            DPRINT("sending query '%s' to adb\n", mWriteBuffer);
            asyncWriter_init(mAsyncWriter, mWriteBuffer, ::strlen(mWriteBuffer), mAdbIo);
            mstate = TO_QUERY_PROPERTY;
            break;
        case TO_READ_PROPERTY:
            size = socket_recv(mAdbSocket, buff, sizeof(buff));
            if (size <0) {
                return;
            }
            mreply.append(buff, size);
            if (size > 0) {
                return;
            }
            //socket closed
            snprintf(buff, sizeof(buff), "%s", mreply.c_str());
            mreply.clear();
            DPRINT("received property mesg from adb host:%s\n", buff);

            if (!strncmp("OKAY", pch, 4)) {
                pch += 4;
            }
            if (!strncmp("KKWT", pch, 4) && mdevs.size()) {
                mwears.push_back(mdevs.back());
                DPRINT("added %s to wear\n", mdevs.back().c_str());
            } else {
                closeAdbConnection();
                openAdbConnection();
                makeQueryMsg(mWriteBuffer, sizeof(mWriteBuffer), mdevs.back().c_str(), TRANSFER);
                asyncWriter_init(mAsyncWriter, mWriteBuffer, ::strlen(mWriteBuffer), mAdbIo);
                DPRINT("sending query '%s' to adb\n", mWriteBuffer);
                mstate = TO_QUERY_WEARABLE_APP_PKG_1;
                break;
            }

            checkQueryCompletion();
            return;
        case TO_READ_WEARABLE_APP_PKG_1:
            status = asyncReader_read(mAsyncReader);
            if (ASYNC_NEED_MORE == status) {
                return;
            } else if (ASYNC_ERROR == status || strncmp("OKAY", mReadBuffer,4)) {
                DPRINT("Error: cannot get OKAY response from ADB server\n");
                mstate = PAIRUP_ERROR;
                return;
            }
            DPRINT("received xfer mesg from adb host:%s\n", mReadBuffer);
            mstate = TO_QUERY_WEARABLE_APP_PKG_2;
            makeQueryMsg(mWriteBuffer, sizeof(mWriteBuffer), mdevs.back().c_str(), WEARABLE_APP_PKG, 
                    " wearablepreview");
            asyncWriter_init(mAsyncWriter, mWriteBuffer, ::strlen(mWriteBuffer), mAdbIo);
            DPRINT("sending query '%s' to adb\n", mWriteBuffer);
            break;
        case TO_READ_WEARABLE_APP_PKG_2:
            size = socket_recv(mAdbSocket, buff, sizeof(buff));
            if (size <0) {
                return;
            }
            mreply.append(buff, size);
            if (size > 0) {
                return;
            }
            //socket closed
            snprintf(buff, sizeof(buff), "%s", mreply.c_str());
            DPRINT("received property mesg from adb host:%s\n", buff);

            if (!strncmp("OKAY", pch, 4)) {
                pch += 4;
            }
            if (mreply.find("wearablepreview") !=  std::string::npos && mdevs.size()) {
                mphones.push_back(mdevs.back());
                DPRINT("added %s to phone\n", mdevs.back().c_str());
            } else if (mreply.find("Error:") !=  std::string::npos && 
                      mreply.find("Is the system running?") !=  std::string::npos) {
                //try again: package manager is not up and running yet
                closeAdbConnection();
                openAdbConnection();
                makeQueryMsg(mWriteBuffer, sizeof(mWriteBuffer), mdevs.back().c_str(), TRANSFER);
                asyncWriter_init(mAsyncWriter, mWriteBuffer, ::strlen(mWriteBuffer), mAdbIo);
                DPRINT("sending query '%s' to adb\n", mWriteBuffer);
                mstate = TO_QUERY_WEARABLE_APP_PKG_1;
                break;
            }
            checkQueryCompletion();
            return;
        case TO_READ_FORWARDIP_OKAY:
            size = socket_recv(mAdbSocket, buff, sizeof(buff)-1);
            DPRINT("received forward mesg from adb host:%s\n", buff);
            closeAdbConnection();
            mstate = FORWARDIP_DONE;
            return;
        default:
            break;
    }

    loopIo_dontWantRead(mAdbIo);
    loopIo_wantWrite(mAdbIo);
}

void PairUpWearPhoneImpl::checkQueryCompletion() {
    mdevs.pop_back();
    closeAdbConnection();
    if (mdevs.size()) { //keep probing
        DPRINT("continue to probe\n");
        openAdbConnection();
        loopIo_wantWrite(mAdbIo);
        makeQueryMsg(mWriteBuffer, sizeof(mWriteBuffer), mdevs.back().c_str(), TRANSFER);
        asyncWriter_init(mAsyncWriter, mWriteBuffer, ::strlen(mWriteBuffer), mAdbIo);
        DPRINT("sending query '%s' to adb\n", mWriteBuffer);
        mstate = TO_QUERY_XFER;
    } else {
        mstate = QUERY_DONE;
        if (mwears.size() && mphones.size()) {
            forwardIp(mphones[0].c_str());
        } else {
            DPRINT("done, cannot find wear-phone\n");
            mstate = PAIRUP_ERROR;
        }
    }
}


void PairUpWearPhoneImpl::onQueryAdb() {
    if (mrestart) {
        restart();
        return;
    }
    if (mAdbSocket < 0 || !mAdbIo) {
        return;
    }

    int status = asyncWriter_write(mAsyncWriter);
    if (ASYNC_ERROR == status) {
        DPRINT("Error: cannot setup port transfer\n");
        mstate = PAIRUP_ERROR;
        return;
    } else if (ASYNC_NEED_MORE == status) {
        return;
    }

    switch (mstate) {
        case TO_QUERY_XFER:
            mReadBuffer[4]='\0';
            asyncReader_init(mAsyncReader, mReadBuffer, 4, mAdbIo);
            mstate = TO_READ_XFER_OKAY;
            break;
        case TO_QUERY_PROPERTY:
            mstate = TO_READ_PROPERTY;
            break;
        case TO_QUERY_WEARABLE_APP_PKG_1:
            mReadBuffer[4]='\0';
            asyncReader_init(mAsyncReader, mReadBuffer, 4, mAdbIo);
            mstate = TO_READ_WEARABLE_APP_PKG_1;
            break;
        case TO_QUERY_WEARABLE_APP_PKG_2:
            mstate = TO_READ_WEARABLE_APP_PKG_2;
            break;
        case TO_QUERY_FORWARDIP: 
            mstate = TO_READ_FORWARDIP_OKAY;
            break;
        default:
            break;
    }

    mreply.clear();
    loopIo_dontWantWrite(mAdbIo);
    loopIo_wantRead(mAdbIo);
}

void PairUpWearPhoneImpl::onConsoleReply() {
    if (mrestart) {
        restart();
        return;
    }
    if (mConsoleSocket < 0 || !mConsoleIo) {
        return;
    }

    char buff[4096]={'\0'};
    const int size = socket_recv(mConsoleSocket, buff, sizeof(buff)-1);
    if (size == 0) {
        loopIo_dontWantRead(mConsoleIo);
    } else if (size > 0) {
        DPRINT("received mesg from console:%s\n", buff);
        snprintf(mWriteBuffer, sizeof(mWriteBuffer), "redir add tcp:5601:5601\nquit\n");
        DPRINT("send to console %s", mWriteBuffer);
        asyncWriter_init(mAsyncWriter, mWriteBuffer, ::strlen(mWriteBuffer), mConsoleIo);
    }
}

void PairUpWearPhoneImpl::onQueryConsole() {
    if (mrestart) {
        restart();
        return;
    }
    if (mConsoleSocket < 0 || !mConsoleIo) {
        DPRINT("ERROR: cannot talk to console\n");
        return;
    }

    int status = asyncWriter_write(mAsyncWriter);
    if (ASYNC_ERROR == status) {
        DPRINT("Error: cannot write to console\n");
        mstate = PAIRUP_ERROR;
        return;
    } else if (ASYNC_NEED_MORE == status) {
        return;
    }
    loopIo_dontWantWrite(mConsoleIo);
    loopIo_dontWantRead(mConsoleIo);
    mstate = FORWARDIP_DONE;
}

//------------------ end of callbacks related to IO


PairUpWearPhoneImpl::PairUpWearPhoneImpl(Looper* looper, int adbHostPort) :
        mLooper(looper), mAdbHostPort(adbHostPort), mAdbSocket(-1),
        mAdbIo(), mConsolePort(-1), mConsoleSocket(-1), mConsoleIo(),
        mdevs(), mwears(), mphones(), mstate(PAIRUP_ERROR), mdevicesmsg(),
        mrestart(0), mAsyncReader(), mAsyncWriter(), mReadBuffer(), mWriteBuffer() {
    assert(looper);
    assert(adbHostPort > 1024);
}

PairUpWearPhoneImpl::~PairUpWearPhoneImpl() {
    closeAdbConnection();
    loopIo_done(mAdbIo);
    closeConsoleConnection();
    loopIo_done(mConsoleIo);
    mLooper = 0;
    mAdbHostPort = -1;
    mConsolePort = -1;
}

void
PairUpWearPhoneImpl::parse(const char* msg, StrVec& tokens)
{
    const char* delims = " \t\r\n";
    char* buf = strdup(msg);
    char* pch = strtok(buf, delims);
    char* prev = NULL;
    while(pch) {
        if (!strcmp("device", pch) && prev) {
                tokens.push_back(prev);
        }
        prev = pch;
        pch = strtok(NULL, delims);
    }

    free(buf);
}

void PairUpWearPhoneImpl::connectWearToPhone(const char* s_devicesupdate_msg) {
    DPRINT("start kicking again\n");
    StrVec devices;
    parse(s_devicesupdate_msg, devices);
    if (devices.size() < 2) {
        DPRINT("Error: cannot handle message: %s\n", s_devicesupdate_msg);
        mstate = PAIRUP_ERROR;
        return;
    }

    mdevs.swap(devices);
    mdevicesmsg = std::string(s_devicesupdate_msg);
    if (mstate !=  PAIRUP_ERROR && mstate !=  FORWARDIP_DONE) {
        mrestart = 1;//delay restart
    } else {
        restart();
    }
    return;
}

void PairUpWearPhoneImpl::restart() {
    mrestart = 0;
    closeConsoleConnection();
    closeAdbConnection();
    mwears.clear();
    mphones.clear();
    assert(mdevicesmsg.size());

    int i = 0;
    int j = mdevs.size()-1;
    while(i < j)
        swap(mdevs[i++], mdevs[j--]);

    openAdbConnection();
    loopIo_wantWrite(mAdbIo);
    makeQueryMsg(mWriteBuffer, sizeof(mWriteBuffer), mdevs.back().c_str(), TRANSFER);
    asyncWriter_init(mAsyncWriter, mWriteBuffer, ::strlen(mWriteBuffer), mAdbIo);
    DPRINT("sending query '%s' to adb\n", mWriteBuffer);
    mstate = TO_QUERY_XFER;
}

void PairUpWearPhoneImpl::forwardIp(const char* phone) {
    const char* emu = "emulator-";
    const int sz = ::strlen(emu);
    if (!strncmp(emu, phone, sz)) {//emulated phone
        //the host-serial:emulator-serial-number:forward command
        //does not work, so we need to work around and connect 
        //to the emulator console and issue command "redir add"
        mConsolePort = atoi(phone + sz);
        assert(mConsolePort >=  5554);
        openConsoleConnection();
        loopIo_wantRead(mConsoleIo);
    }
    else { //usb phone
        //need to close socket first, because we have previously 
        //set it up for forwarding
        closeAdbConnection();
        //directly tell adb following command
        //"host-serial:phone-serial-number:forward:tcp:5601;5601"
        openAdbConnection();
        loopIo_wantWrite(mAdbIo);
        makeQueryMsg(mWriteBuffer, sizeof(mWriteBuffer), phone, FORWARDIP);
        DPRINT("sending query '%s' to adb\n", mWriteBuffer);
        asyncWriter_init(mAsyncWriter, mWriteBuffer, ::strlen(mWriteBuffer), mAdbIo);
        mstate = TO_QUERY_FORWARDIP;
    }
}

//-----------------------            helper methods
void PairUpWearPhoneImpl::openConsoleConnection()
{
    DPRINT("\nconnecting to console %d\n", mConsolePort);

    openConnection(mConsolePort, mConsoleSocket, mConsoleIo);
    loopIo_init(mConsoleIo, mLooper, mConsoleSocket, _query_console, this);
}

void PairUpWearPhoneImpl::closeConsoleConnection() {
    closeConnection(mConsoleSocket, mConsoleIo);
}

void PairUpWearPhoneImpl::closeAdbConnection() {
    closeConnection(mAdbSocket, mAdbIo);
}

void PairUpWearPhoneImpl::openAdbConnection() {
    openConnection(mAdbHostPort, mAdbSocket, mAdbIo);
    loopIo_init(mAdbIo, mLooper, mAdbSocket, _query_adb, this);
}

void PairUpWearPhoneImpl::closeConnection(int& so, LoopIo* io) {
    if (so >=  0) {
        char buff[1024];
        snprintf(buff, sizeof(buff), "cleanedup socket %d \n", so);
        socket_close(so);
        so = -1;
        DPRINT("%s", buff);
        snprintf(buff, sizeof(buff), "cleanedup io connection %p \n", io);
        loopIo_dontWantRead(io);
        loopIo_dontWantWrite(io);
        DPRINT("%s", buff);
    }
}

void PairUpWearPhoneImpl::openConnection(int port, int& so, LoopIo* io) {
    if (so < 0) {
        so = socket_loopback_client(port, SOCKET_STREAM);
        if (so >=  0) {
            socket_set_nonblock(so);
        } else {
            DPRINT("failed to open up connection to port %d\n", port);
            return;
        }
    }

    DPRINT("\n\n");
    DPRINT("succeeded to open up connection to port %d\n", port);
}

void PairUpWearPhoneImpl::makeQueryMsg(char buff[], int sz, const char* serial, 
        MSG_TYPE type, const char* property) {
    assert(serial);
    char buf2[1024];
    switch(type) {
        case TRANSFER:
            snprintf(buf2, sizeof(buf2), "host:transport:%s", serial);
            break;
        case PROPERTY:
            snprintf(buf2, sizeof(buf2), "shell:getprop %s", property);
            break;
        case WEARABLE_APP_PKG:
            snprintf(buf2, sizeof(buf2), "shell:pm list packages %s", property);
            break;
        case FORWARDIP:
            snprintf(buf2, sizeof(buf2), "host-serial:%s:forward:tcp:5601;tcp:5601", serial);
            break;
        default:
            buf2[0] = '\0';
            break;
    }
    snprintf(buff, sz, "%04x%s", (unsigned)(::strlen(buf2)), buf2);
    assert(::strlen(buff) < 1024);
}

PairUpWearPhone::PairUpWearPhone(Looper* looper, int adbHostPort) :
        mPairUpWearPhoneImpl(new PairUpWearPhoneImpl(looper, adbHostPort)) {
}

PairUpWearPhone::~PairUpWearPhone() {
    delete mPairUpWearPhoneImpl;
}

void PairUpWearPhone::connectWearToPhone(const char* devices) {
    mPairUpWearPhoneImpl->connectWearToPhone(devices);
}

} // end of namespace wear
} // end of namespace android
