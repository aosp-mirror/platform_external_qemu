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

#include "android/base/async/Looper.h"
#include "android/base/Limits.h"
#include "android/base/Log.h"
#include "android/base/sockets/SocketDrainer.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/wear-agent/WearAgent.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace android::base;

void print_help(const char* progname);

static void _on_time_up (void* opaque) {
    Looper* looper = static_cast<Looper*>(opaque);
    printf("stop the wear agent now !\n");
    looper->forceQuit();
}

bool parse_arguments(int argc, char** argv, int& adbHostPort, int& secondsToRun);

int main(int argc, char** argv)
{
    int adbHostPort = 5037;
    int secondsToRun = 0;
    if (!parse_arguments(argc, argv, adbHostPort, secondsToRun)) {
        print_help(argv[0]);
        return 1;
    }

    Looper* mainLooper = Looper::create();

    // Enclose in a block so that agent can cleanup before
    // looper is freed, otherwise agent cannot cleanup properly
    {
        Looper::Timer* timer = NULL;
        if (secondsToRun > 0) {
            timer = mainLooper->createTimer(_on_time_up, mainLooper);
            const Looper::Duration dl = 1000 * secondsToRun;
            timer->startRelative(dl);
        }
        android::wear::WearAgent agent(mainLooper, adbHostPort);
        SocketDrainer socketDrainer(mainLooper);

        mainLooper->run();

        if (secondsToRun > 0) {
            delete timer;
            timer = NULL;
        }
    }

    delete mainLooper;

    return 0;
}

bool parse_arguments(int argc, char** argv, int& adbHostPort, int& secondsToRun) {
    if (argc == 1) return true;
    int i=0;
    while (1) {
        ++ i;
        if (i >= argc) {
            break;
        }
        if (!strcmp("-p", argv[i])) {
            ++i;
            if (i >= argc) {
                printf("Error: missing adb host port number.\n");
                return false;
            }
            if ((adbHostPort = atoi(argv[i])) < 5037) {
                printf("Error: wrong adb host port: '%s', should be equal to"
                        " or greater than 5037.\n", argv[i]);
                return false;
            }
        } else if (0 == strcmp("-t", argv[i])) {
            ++i;
            if (i >= argc) {
                printf("Error: missing number of seconds to run.\n");
                return false;
            }
            secondsToRun = atoi(argv[i]);
            if ((secondsToRun < 0) || (secondsToRun == 0 &&  0 != strcmp("0", argv[i]))) {
                printf("Error: wrong argument for -t option: '%s',"
                        " should be equal to or greater than 0.\n", argv[i]);
                return false;
            }
        } else {
            printf("Error: wrong option: '%s'\n", argv[i]);
            return false;
        }
    }
    return true;
}

void print_help(const char* progname) {
    printf("Android Wear Connection Agent version 1.0.0\n");
    printf("usage:");
    printf("\t%s     [options]\n", progname);
    printf(" -p                            - Port of adb server (default: 5037)\n");
    printf(" -t                            - Number of seconds the Wear Connection Agent should"
            " run (default 0: non-stop)\n");
}
