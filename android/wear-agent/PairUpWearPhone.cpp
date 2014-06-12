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

//have to include following to get rid of "INT64_MAX" definition issue

extern "C" {
#include <android/base/Limits.h>
#include <android/base/String.h>
#include "android/android.h"
#include "android/looper.h"
#include "android/sockets.h"
//bohu: notes: following is for LoopIo* related stuff
#include "android/async-utils.h"
#include "android/utils/debug.h"
#include "android/utils/list.h"
#include "android/utils/misc.h"
#define  QB(b, s)  quote_bytes((const char*)b, (s < 32) ? s : 32)
}

#include "android/wear-agent/PairUpWearPhone.h"
//bohu: notes: to use global namespace functions, use "::".
//for example, ::memmove or ::strlen
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


namespace android {
namespace wear {

extern "C" {

//talks to adb on the host computer
static void
_query_adb(void* opaque, int fd, unsigned events) {
    android::wear::PairUpWearPhone * pairup = 
        (android::wear::PairUpWearPhone*)opaque;
    assert(pairup);

    if ((events & LOOP_IO_READ) != 0) {
        pairup->onAdbReply();
    }
    if ((events & LOOP_IO_WRITE) != 0) {
        pairup->onQueryAdb();
    }
}

//talks to emulator console
//similar to "telnet localhost console_port"
static void
_query_console(void* opaque, int fd, unsigned events) {
    android::wear::PairUpWearPhone * pairup = 
        (android::wear::PairUpWearPhone*)opaque;
    assert(pairup);

    if ((events & LOOP_IO_READ) != 0) {
        pairup->onConsoleReply();
    }
    if ((events & LOOP_IO_WRITE) != 0) {
        pairup->onQueryConsole();
    }
}

}


//-------------- callbacks for IO with Adb  and for IO with Console
void
PairUpWearPhone::onAdbReply()
{
    if (mrestart) {
        restart();
        return;
    }

    if (madbso < 0 || !madbio)
        return;
    
    if(mdevs.empty() && mstate != TO_READ_FORWARDIP_OKAY)
        return;

    //printf("pairup->madbio->impl: %p\n", madbio->impl);
    int size=0;
    char buff[4096]={'\0'};
    buff[0]='\0';
    char* pch=buff;
    switch(mstate) {
        case TO_READ_XFER_OKAY:
            size=socket_recv(madbso, buff, sizeof(buff));
            buff[size>=0?size:0]='\0';
            printf("received xfer mesg from adb host:%s\n", buff);
            fflush(stdout);
            //after xfer, socket is still open
            
            mstate = TO_QUERY_PROPERTY;
            break;
        case TO_READ_PROPERTY:
            size=socket_recv(madbso, buff, sizeof(buff));
            if (size <0)
                return;
            mreply += std::string(buff);
            if (size > 0)
                return;
            //socket closed
            strncpy(buff, mreply.c_str(), sizeof(buff)); mreply.clear();
            printf("received property mesg from adb host:%s\n", QB(buff, ::strlen(buff)));
            fflush(stdout);

            if (!strncmp("OKAY", pch, 4))
                pch += 4;
            if (!strncmp("KKWT", pch, 4)) {
                mwears.push_back(mdevs.back());
                printf("added %s to wear\n", mdevs.back().c_str());
            }
            else {
                closeAdbConnection();
                openAdbConnection();
                loopIo_wantWrite(madbio);
                mstate = TO_QUERY_WEARABLE_APP_PKG_1;
                break;
            }

            checkQueryCompletion();
            return;
        case TO_READ_WEARABLE_APP_PKG_1:
            size=socket_recv(madbso, buff, sizeof(buff));
            buff[size>=0?size:0]='\0';
            printf("received xfer mesg from adb host:%s\n", buff);
            fflush(stdout);
            mstate = TO_QUERY_WEARABLE_APP_PKG_2;
            break;
        case TO_READ_WEARABLE_APP_PKG_2:
            size=socket_recv(madbso, buff, sizeof(buff));
            if (size <0)
                return;
            mreply += std::string(buff);
            if (size > 0)
                return;
            if (mreply.find("\r\n") == std::string::npos)
                return;
            //socket closed
            strncpy(buff, mreply.c_str(), sizeof(buff));
            printf("received property mesg from adb host:%s\n", buff);
            fflush(stdout);

            if (!strncmp("OKAY", pch, 4))
                pch += 4;
            if (mreply.find("wearablepreview") != std::string::npos) {
                mphones.push_back(mdevs.back());
                printf("added %s to phone\n", mdevs.back().c_str());
            } else if (mreply.find("Error:") != std::string::npos && 
                      mreply.find("Is the system running?") != std::string::npos) {
                //try again: package manager is not up and running yet
                closeAdbConnection();
                openAdbConnection();
                loopIo_wantWrite(madbio);
                mstate = TO_QUERY_WEARABLE_APP_PKG_1;
                break;
            }
            mreply.clear();
            checkQueryCompletion();
            return;
        case TO_READ_FORWARDIP_OKAY:
            size=socket_recv(madbso, buff, sizeof(buff));
            buff[size>=0?size:0]='\0';
            printf("received forward mesg from adb host:%s\n", buff);
            fflush(stdout);
            closeAdbConnection();
            mstate = FORWARDIP_DONE;
            return;
        default:
            break;
    }

    loopIo_dontWantRead(madbio);
    loopIo_wantWrite(madbio);
}

void
PairUpWearPhone::checkQueryCompletion() {
    fflush(stdout);
    mdevs.pop_back();
    closeAdbConnection();
    if (mdevs.size()) { //keep probing
        printf("continue to probe\n");
        fflush(stdout);
        openAdbConnection();
        loopIo_wantWrite(madbio);
        mstate = TO_QUERY_XFER;
    } else {
        mstate = QUERY_DONE;
        if (mwears.size() && mphones.size())
            forwardip(mphones[0].c_str());
        else {
            printf("done, cannot find wear-phone\n");
            fflush(stdout);
            mstate=PAIRUP_ERROR;
        }
    }
}


void
PairUpWearPhone::onQueryAdb()
{
    if (mrestart) {
        restart();
        return;
    }

    if (madbso < 0 || !madbio)
        return; 
    if(mdevs.empty() && mstate != TO_QUERY_FORWARDIP)
        return;
    //printf("pairup->madbio->impl: %p\n", madbio->impl);

    int size=0;
    char buff[4096]={'\0'};
    switch (mstate) {
        case TO_QUERY_XFER:
            if (mdevs.empty())
                return;
            make_query_msg(buff, sizeof(buff), mdevs.back().c_str(), TRANSFER );
            size=socket_send(madbso, buff, ::strlen(buff));
            printf("sending query '%s' to adb\n", buff);
            if (size <= 0) {
                printf("Error: cannot setup port transfer\n");
                fflush(stdout);
                mstate = PAIRUP_ERROR;
                return;
            }

            mstate = TO_READ_XFER_OKAY;
            break;
        case TO_QUERY_PROPERTY:
            make_query_msg(buff, sizeof(buff), mdevs.back().c_str(), PROPERTY, "ro.build.version.release");
            size=socket_send(madbso, buff, ::strlen(buff));
            printf("sending query '%s' to adb\n", buff);
            if (size <= 0) {
                printf("Error: cannot read device property \n");
                fflush(stdout);
                mstate = PAIRUP_ERROR;
                return;
            }
            mreply.clear();
            mstate = TO_READ_PROPERTY;
            break;
        case TO_QUERY_WEARABLE_APP_PKG_1:
            make_query_msg(buff, sizeof(buff), mdevs.back().c_str(), TRANSFER );
            size=socket_send(madbso, buff, ::strlen(buff));
            printf("sending query '%s' to adb\n", buff);
            
            mreply.clear();
            mstate = TO_READ_WEARABLE_APP_PKG_1;
            break;
        case TO_QUERY_WEARABLE_APP_PKG_2:
            make_query_msg(buff, sizeof(buff), mdevs.back().c_str(), WEARABLE_APP_PKG, 
                    " wearablepreview");
            size=socket_send(madbso, buff, ::strlen(buff));
            printf("sending query '%s' to adb\n", buff);
            if (size <= 0) {
                printf("Error: cannot read device property \n");
                fflush(stdout);
                mstate = PAIRUP_ERROR;
                return;
            }
            mreply.clear();
            mstate = TO_READ_WEARABLE_APP_PKG_2;
            break;
        case TO_QUERY_FORWARDIP: 
            //ask adb host to forward tcp:5601 to phone's 5601 port
            make_query_msg(buff, sizeof(buff), mphones[0].c_str(), FORWARDIP);
            size=socket_send(madbso, buff, ::strlen(buff));
            printf("sending query '%s' to adb\n", buff);
            fflush(stdout);
            if (size <= 0) {
                printf("Error: cannot setup port forwarding\n");
                fflush(stdout);
                mstate = PAIRUP_ERROR;
                break;
            }

            mstate = TO_READ_FORWARDIP_OKAY;
            break;
        default:
            break;
    }

    //printf("1pairup->madbio->impl: %p\n", madbio->impl);
    loopIo_dontWantWrite(madbio);
    //printf("2pairup->madbio->impl: %p\n", madbio->impl);
    loopIo_wantRead(madbio);
}

void
PairUpWearPhone::onConsoleReply() {
    if (mrestart) {
        restart();
        return;
    }
    if (mconsoleso < 0 || !mconsoleio)
        return;
    char buff[4096];
    const int size=socket_recv(mconsoleso, buff, sizeof(buff));
    if (size==0) {
        loopIo_dontWantRead(mconsoleio);
    }
    if (size > 0) {
         printf("received mesg from console:%s\n", buff);
         fflush(stdout);
         loopIo_wantWrite(mconsoleio);
    }
}

void
PairUpWearPhone::onQueryConsole() {
    if (mrestart) {
        restart();
        return;
    }
    if (mconsoleso < 0 || !mconsoleio) {
        printf("ERROR: cannot talk to console\n");
        fflush(stdout);
        return;
    }
    char buff[4096];
    
    snprintf(buff, sizeof(buff), "redir add tcp:5601:5601\n");
    socket_send(mconsoleso, buff, ::strlen(buff));
    printf("send to console %s", buff);
    fflush(stdout);
    
    snprintf(buff, sizeof(buff), "quit\n");
    socket_send(mconsoleso, buff, ::strlen(buff));
    printf("send to console %s", buff);
    fflush(stdout);
    //we are done writing, but just keep the listing channel open
    loopIo_dontWantWrite(mconsoleio);
    loopIo_dontWantRead(mconsoleio);
    mstate = FORWARDIP_DONE;
}

//------------------ end of callbacks related to IO


PairUpWearPhone::PairUpWearPhone(Looper* looper, int adbhostport)
:mlooper(looper)
,madbport(adbhostport)
,madbso(-1)
,madbio(0)
,mconsoleport(-1)
,mconsoleso(-1)
,mconsoleio(0) 
,mdevs() 
,mwears() 
,mphones() 
,mstate(PAIRUP_ERROR) 
,mdevicesmsg() 
,mrestart(0) 
{
    assert(looper);
    assert(adbhostport > 1024);
}

PairUpWearPhone::~PairUpWearPhone() {
    closeAdbConnection();
    closeConsoleConnection();
    mlooper=0;
    madbport=-1;
    mconsoleport=-1;
}

void
PairUpWearPhone::parse(const char* msg, StrVec& tokens)
{
    const char* delims = " \t\r\n";
    char* buf = strdup(msg);
    char* pch = strtok(buf, delims);
    char* prev=NULL;
    while(pch) {
        if (!strcmp("device", pch) && prev)
                tokens.push_back(prev);
        prev = pch;
        pch = strtok(NULL, delims);
    }

    free(buf);
}

void
PairUpWearPhone::kickoff(const char* s_devicesupdate_msg)
{
    printf("start kicking again\n");
    fflush(stdout);
    StrVec devices;
    parse(s_devicesupdate_msg, devices);
    if (devices.size() < 2) {
        printf("Error: cannot handle message: %s\n", s_devicesupdate_msg);
        mstate = PAIRUP_ERROR;
        return;
    }

    mdevs.swap(devices);
    mdevicesmsg = std::string(s_devicesupdate_msg);
    if (mstate != PAIRUP_ERROR && mstate != FORWARDIP_DONE)
        mrestart=1;//delay restart
    else
        restart();
    return;
}

void
PairUpWearPhone::restart() {
    mrestart=0;
    closeConsoleConnection();
    closeAdbConnection();
    //mdevs.clear();
    mwears.clear();
    mphones.clear();
    assert(mdevicesmsg.size());
    //right now, can only handle one wear with one phone

    int i=0;
    int j=mdevs.size()-1;
    while(i < j)
        swap(mdevs[i++], mdevs[j--]);

    openAdbConnection();
    loopIo_wantWrite(madbio);
    mstate = TO_QUERY_XFER;
}

void
PairUpWearPhone::forwardip(const char* phone)
{
    const char* emu = "emulator-";
    const int sz = ::strlen(emu);
    if (!strncmp(emu, phone, sz)) {//emulated phone
        //the host-serial:emulator-serial-number:forward command
        //does not work, so we need to work around and connect 
        //to the emulator console and issue command "redir add"
        mconsoleport = atoi(phone + sz);
        assert(mconsoleport >= 5554);
        openConsoleConnection();
        loopIo_wantRead(mconsoleio);
        //loopIo_wantWrite(mconsoleio);
    }
    else { //usb phone
        //need to close socket first, because we have previously 
        //set it up for forwarding
        closeAdbConnection();
        //directly tell adb following command
        //"host-serial:phone-serial-number:forward:tcp:5601;5601"
        openAdbConnection();
        loopIo_wantWrite(madbio);
        mstate = TO_QUERY_FORWARDIP;
    }
}

//-----------------------            helper methods
void
PairUpWearPhone::openConsoleConnection()
{
    printf("\n\n connecting to console %d\n", mconsoleport);
    fflush(stdout);

    openConnection(mconsoleport, mconsoleso, mconsoleio);
    loopIo_init(mconsoleio, mlooper, mconsoleso, _query_console, this);
}

void
PairUpWearPhone::closeConsoleConnection() {
    closeConnection(mconsoleso, mconsoleio);
}

void
PairUpWearPhone::closeAdbConnection() {
    closeConnection(madbso, madbio);
}

void
PairUpWearPhone::openAdbConnection() {
    openConnection(madbport, madbso, madbio);
    loopIo_init(madbio, mlooper, madbso, _query_adb, this);
}

void
PairUpWearPhone::closeConnection(int& so, LoopIo* &io)
{
    if (so >= 0) {
        char buff[1024];
        snprintf(buff, sizeof(buff), "cleanedup socket %d \n", so);
        socket_close(so);
        so=-1;
        printf("%s", buff);
        fflush(stdout);
    }
    if (io) {
        char buff[1024];
        snprintf(buff, sizeof(buff), "cleanedup io connection %p \n", io);
        loopIo_dontWantRead(io);
        loopIo_dontWantWrite(io);
        loopIo_done(io);
        AFREE(io);
        io=0;
        printf("%s", buff);
        fflush(stdout);
    }
}

void
PairUpWearPhone::openConnection(int port, int& so, LoopIo* &io) {
    if (so < 0) {
        so = socket_loopback_client(port, SOCKET_STREAM);
        if (so >= 0)
            socket_set_nonblock(so);
        else {
            printf("failed to open up connection to port %d\n", port);
            fflush(stdout);
            return;
        }
    }

    if (!io) {
        io = (LoopIo*)calloc(sizeof(*io), 1);
        assert(io);
        assert(so >= 0);
    }
    printf("\n\n");
    printf("succeeded to open up connection to port %d\n", port);
    fflush(stdout);
}

void
PairUpWearPhone::make_query_msg(char buff[], int sz, const char* serial, MSG_TYPE type, const char* property)
{
    assert(serial);
    char buf2[1024];
    switch(type) {
        case TRANSFER:
            //snprintf(buf2, sizeof(buf2), "host-serial:%s:shell:getprop %s", serial, property);
            snprintf(buf2, sizeof(buf2), "host:transport:%s", serial);
            break;
        case PROPERTY:
            //snprintf(buf2, sizeof(buf2), "host-serial:%s:shell:getprop %s", serial, property);
            snprintf(buf2, sizeof(buf2), "shell:getprop %s", property);
            break;
        case WEARABLE_APP_PKG:
            //snprintf(buf2, sizeof(buf2), "host-serial:%s:shell:getprop %s", serial, property);
            snprintf(buf2, sizeof(buf2), "shell:pm list packages %s", property);
            break;
        case FORWARDIP: //host-usb:forward:tcp:5601;tcp:5601
            snprintf(buf2, sizeof(buf2), "host-serial:%s:forward:tcp:5601;tcp:5601", serial);
            break;
        default:
            buf2[0]='\0';
            break;
    }
    snprintf(buff, sz, "%04x%s", (unsigned)(::strlen(buf2)), buf2);
    assert(::strlen(buff) < 1024);
}

} //namespace wear
}


