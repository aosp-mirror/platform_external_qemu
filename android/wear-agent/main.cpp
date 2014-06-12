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
#include "android/wear-agent/WearAgent.h"

extern "C" {
#include "android/looper.h"
#include "android/sockets.h"
#include "android/utils/debug.h"
extern void start_socket_drainer(Looper* looper);
extern void socket_drainer_add(int fd);
extern void stop_socket_drainer();
extern void start_wear_agent(Looper* looper);
extern void stop_wear_agent();
}

#include <stdio.h>
#include <stdlib.h>


void print_help(const char* progname);
void test_socket_drainer();

static void _on_time_up (void* opaque) {
    Looper* looper=(Looper*)opaque;
    printf("stop the wear agent now !\n");
    looper_forceQuit(looper);
}

int main(int argc, char** argv)
{
    int secondsToExitAgent = 0;
    if (argc > 2) {
        print_help(argv[0]);
        return 0;
    } else if (argc == 2) {
        secondsToExitAgent = atoi(argv[1]);
        if (secondsToExitAgent == 0) {
            print_help(argv[0]);
            return 0;
        }
    }
#ifdef _WIN32
    socket_init();
#endif

    VERBOSE_ENABLE(adb);

    // Test socket_drainer before looper is available
    test_socket_drainer();

    Looper* mainLooper = looper_newGeneric();


    // Enclose in a block so that agent can cleanup before
    // looper is freed, otherwise agent cannot cleanup properly
    {
        LoopTimer           agentTimer[1];
        if (secondsToExitAgent > 0) {
            loopTimer_init(agentTimer, mainLooper, _on_time_up, mainLooper);
            const Duration dl = 1000*secondsToExitAgent;
            loopTimer_startRelative(agentTimer, dl);
        }
        android::wear::WearAgent agent(mainLooper);
        start_socket_drainer(mainLooper);

        // Test socket_drainer after looper is available
        test_socket_drainer();

        looper_run(mainLooper);
        stop_socket_drainer();
        if (secondsToExitAgent > 0) {
            loopTimer_done(agentTimer);
        }
    }

    looper_free(mainLooper);


    return 0;
}

void test_socket_drainer() {
    int socket1 = socket_loopback_client(5037, SOCKET_STREAM);
    if (socket1 >= 0) {
        char buff[1024] = {'\0'};
        // Correct message: "0012host:track-devices"
        snprintf(buff, sizeof(buff), "0012host:track-devickk"); // intentioanlly wrong
        socket_send(socket1, buff, ::strlen(buff));
        socket_drainer_add(socket1);
    }
}

void print_help(const char* progname) {
    if (!progname) {
        return;
    }
    printf("Android Wear Connection Agent version 1.0.0\n");
    printf("Usage:\n");
    printf("\t%s     #run forever\n", progname);
    printf("\t%s [n] #run n seconds and stop\n", progname);
}
