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
extern "C" {
#include "android/utils/debug.h"
#include "android/looper.h"
#include "android/sockets.h"
extern void start_socket_drain_mgr(Looper* looper);
extern void stop_socket_drain_mgr();
extern void start_wear_agent(Looper* looper);
extern void stop_wear_agent();
extern void socket_drain_mgr_add(int fd);
}

#include <stdio.h>


void print_help(const char* progname);
void test_socket_drain_mgr();

int main(int argc, char** argv)
{
    if (argc > 1) {
        print_help(argv[0]);
        return 0;
    }
#ifdef _WIN32
    socket_init();
#endif

    VERBOSE_ENABLE(adb);

    //test socket_drain_mgr before looper is available
    test_socket_drain_mgr();

    Looper* mainLooper = looper_newGeneric();

    android::wear::WearAgent agent(mainLooper);
    start_socket_drain_mgr(mainLooper);

    //test socket_drain_mgr after looper is available
    test_socket_drain_mgr();
    
    looper_run(mainLooper);
    looper_free(mainLooper);
    
    stop_socket_drain_mgr();

    return 0;
}

void test_socket_drain_mgr() {
    int socket1 = socket_loopback_client(5037, SOCKET_STREAM);
    if (socket1 >= 0) {
        char buff[4096]={'\0'};
        //correct message: "0012host:track-devices"
        snprintf(buff, sizeof(buff), "0012host:track-devickk"); //intentioanlly wrong
        socket_send(socket1, buff, ::strlen(buff));
        socket_drain_mgr_add(socket1);
    }
}

void 
print_help(const char* progname) {
    if (!progname) {
        return;
    }
    printf("Android Wear Connection Agent version 1.0.0\n");
    printf("Usage:\n");
    printf("\t%s\n", progname);
}
