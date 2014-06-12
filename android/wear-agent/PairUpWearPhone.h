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

#ifndef ANDROID_PAIR_UP_WEAR_PHONE_H
#define ANDROID_PAIR_UP_WEAR_PHONE_H

#include <string>
#include <vector>

struct Looper;
struct LoopIo;
struct LoopTimer;

namespace android {
namespace wear {

class PairUpWearPhone {

public:
    PairUpWearPhone(Looper* looper, int adbhostport=5037);
    ~PairUpWearPhone();
public:
    void kickoff(const char* s_devicesupdate_msg);
    void onQueryAdb();
    void onQueryConsole();
    void onAdbReply();
    void onConsoleReply();
private:

    Looper*     mlooper;
    int         madbport;
    int         madbso;
    LoopIo*     madbio;
    int         mconsoleport;
    int         mconsoleso;
    LoopIo*     mconsoleio;

    typedef std::vector<std::string> StrVec;

    StrVec      mdevs;
    StrVec      mwears;
    StrVec      mphones;

    enum COMM_STATE {
         PAIRUP_ERROR=0
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

private:
    void openConnection(int port, int& socketfd, LoopIo* &io);
    void closeConnection(int& socketfd, LoopIo* &io);
    void openAdbConnection();
    void closeAdbConnection();
    void openConsoleConnection();
    void closeConsoleConnection();
    void checkQueryCompletion();

    enum MSG_TYPE {
         TRANSFER=1
        ,FORWARDIP
        ,PROPERTY
        ,WEARABLE_APP_PKG
    };
    void make_query_msg(char buff[], int sz, const char* serial, MSG_TYPE type, const char* property=NULL);
    void parse(const char* msg, StrVec& tokens);

    void forwardip(const char* phone_serial_number);

    void restart();
};

} //namespace wear
}

#endif
