// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/utils/adb_start_server.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "android/utils/sockets.h"

static int getPortNumberOfAdbServerOnHost() {
    int android_adb_port = 5037;
    char* adb_host_port_str = getenv( "ANDROID_ADB_SERVER_PORT" );
    if ( adb_host_port_str && strlen( adb_host_port_str ) > 0 ) {
        android_adb_port = (int) strtol( adb_host_port_str, NULL, 0 );
        if ( android_adb_port <= 0 ) {
            fprintf(stderr, "env var ANDROID_ADB_SERVER_PORT must be a number > 0. Got \"%s\"\n",
                    adb_host_port_str );
            android_adb_port = 5037;
        }
    }
    return android_adb_port;
}

void check_and_start_adb_server() {
    // ping adb server first
    // 1. get the adb port number, it may not be 5037
    int adb_server_port = getPortNumberOfAdbServerOnHost();
    // 2. init a timeout connect

    int t100Ms = 100;
    int s = socket_loopback_client_timeout(adb_server_port, SOCKET_STREAM, t100Ms);
    if (s < 0) {
        fprintf(stderr, "starting adb server...\n");
        system("adb start-server");
    } else {
        fprintf(stderr, "adb server is already running\n");
    }
    //start it if it is not there yet
    // 3. start it at the specific port number
}
