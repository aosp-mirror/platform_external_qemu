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

//have to include following to get rid of "INT64_MAX" definition issue

extern "C" {
#include "android/base/Limits.h"
#include "android/looper.h"
#include "android/sockets.h"
#include "android/async-utils.h"
#include "android/utils/debug.h"
#include "android/utils/list.h"
#include "android/utils/misc.h"
#define  QB(b, s)  quote_bytes((const char*)b, (s < 32) ? s : 32)
}

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "android/wear-agent/PairUpWearPhone.h"

namespace android {
namespace wear {
extern "C" {
static void
_talk_to_adb_host(void* opaque, int fd, unsigned events) {
    android::wear::WearAgent* agent = (android::wear::WearAgent*)opaque;
    assert(agent);

    if ((events & LOOP_IO_READ) != 0) {
        agent->onDevicesUpdateFromHost();
    }
    if ((events & LOOP_IO_WRITE) != 0) {
        agent->registerForDevicesUpdate();
    }
}

static void
_connect_to_adb_host(void* opaque) {
    android::wear::WearAgent* agent = (android::wear::WearAgent*)opaque;
    assert(agent);
    agent->makeConnection();
}

}

using namespace android::wear;

WearAgent::WearAgent(Looper* looper, int adbhostport)
:mport(adbhostport)
,mso(-1)
,mlooper(looper)
,mio(0)
,mtimer(0)
,msec(2)//wait for two seconds
,mpairup(new PairUpWearPhone(looper, adbhostport)) 
{
    assert(adbhostport >1024);
    assert(looper);
    mtimer = (LoopTimer*)calloc(sizeof(*mtimer), 1);
    loopTimer_init(mtimer, mlooper, _connect_to_adb_host, this);

    connect_later();
}


void
WearAgent::makeConnection() {
    //create a listening socket on localhost:mport to receive updates of 
    //devices from adb server on host-machine.
    if (mso < 0) {
        mso = socket_loopback_client(mport, SOCKET_STREAM);
        printf("conneting to port %d return %d\n", mport, mso);
        fflush(stdout);
        if (mso >= 0)
            socket_set_nonblock(mso);
        else {
            cleanupConnection();
            connect_later();
            return;
        }
    }

    if (!mio) {
        mio = (LoopIo*)calloc(sizeof(*mio), 1);
        assert(mio);
        assert(mso >= 0);
        loopIo_init(mio, mlooper, mso, _talk_to_adb_host, this);
        loopIo_wantWrite(mio);
    }
}

WearAgent::~WearAgent() {
    delete mpairup;
    mpairup=0;
    cleanupConnection();
    loopTimer_done(mtimer);
    AFREE(mtimer);
    mtimer=0;
}

void
WearAgent::connect_later() {
    if (!mtimer)
        return;

        
    printf("Warning: adb server is not responding,"
            "will connect again in %d seconds. \n", msec);
    fflush(stdout);
    
    const Duration dl = looper_now(mlooper) + 1000*msec;
    loopTimer_startAbsolute(mtimer, dl);
}

void
WearAgent::cleanupConnection() {
    if (mso >= 0) {
        socket_close(mso);
        mso=-1;
    }
    if (mio) {
        loopIo_done(mio);
        AFREE(mio);
        mio=0;
    }
}

void
WearAgent::registerForDevicesUpdate() {
    if (mso < 0 || !mio)
        return;
    //tell adb-host that we want to listen for devices update
    char buff[4096]={'\0'};
    snprintf(buff, sizeof(buff), "0012host:track-devices");
    const int size=socket_send(mso, buff, ::strlen(buff));
    if (size <= 0)
    {
        printf("Error: cannot register for device udpate from adb server.\n");
        fflush(stdout);
        loopIo_dontWantWrite(mio);
        cleanupConnection();
        connect_later();
    } else {
        printf("send to adb host: %s\n", buff);
        loopIo_dontWantWrite(mio);
        loopIo_wantRead(mio);
    }
}

void
WearAgent::onDevicesUpdateFromHost() {
    if (mso < 0 || !mio)
        return;

    //read the devices udpate from adb-host, and handle it
    char buff[4096]={'\0'};
    const int size = socket_recv(mso, buff, sizeof(buff));
    if (size <= 0) {
        printf("Error: cannot read sockt\n");
        fflush(stdout);
        loopIo_dontWantRead(mio);
        cleanupConnection();
        connect_later();
    }
    else {
        //TODO: handle the case when only part of the
        //devices update message is received from adb host;
        //actually there wont be more than 4096 bytes from adb host
        //so, no need to worry
        buff[size] = '\0';
        char* pb = buff;
        printf("read mesg from localhost:%d\n'%s'\n", mport, QB(buff,size));
        printf("=read mesg from localhost:%d\n'%s'\n", mport, buff);
        if (size >= 4 && !strncmp("OKAY", buff, 4))
            pb += 4;
        pb += 4;//remove first 4 chars
        printf("final message: '%s'\n", QB(pb, ::strlen(pb)));
        printf("=final message: '%s'\n", pb);
        loopTimer_stop(mtimer); 
        mpairup->kickoff(pb);
    }
}

}  // namespace wear
} 
