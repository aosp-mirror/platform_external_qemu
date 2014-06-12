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
#include "android/wear-agent/PairUpWearPhone_unittest.h"
extern "C" {
#include "android/looper.h"
#include "android/sockets.h"
#include "android/utils/debug.h"
#include <sys/unistd.h>
extern void start_socket_drainer(Looper* looper);
extern void socket_drainer_add(int fd);
extern void stop_socket_drainer();
}
#include <gtest/gtest.h>
#define  DPRINT(...)  do {  if (VERBOSE_CHECK(adb)) dprintn(__VA_ARGS__); } while (0)

namespace android {
namespace wear {

int testSendToSocket(int socketFd, const char* message) {
    DPRINT("---sending message:%s\n", message);
    int size = strlen(message);
    const char* ptr = message;
    while (size > 0) {
        int ret = socket_send(socketFd, ptr, size);
        if (ret < 0 && errno == EWOULDBLOCK) {
            continue;
        } else if (ret <= 0) {
            DPRINT("---failure\n", message);
            return -1;
        } else {
            ptr += ret;
            size -= ret;
        }
    }
    DPRINT("---success\n", message);
    return 0;
}

int testReceiveMessageFromSocket(int socketFd, char* ptr, int size) {
    while (size > 0) {
        int ret = socket_recv(socketFd, ptr, size);
        if (ret < 0 && errno == EWOULDBLOCK) {
            continue;
        } else if (ret <= 0) {
            return -1;
        } else {
            ptr += ret;
            size -= ret;
        }
    }
    return 0;
}

int testExpectMessageFromSocket(int socketFd, const char* message, int header) {
    int size=0;
    int error=0;
    if (header) {
        char buf[1024] = {'\0'};
        error = testReceiveMessageFromSocket(socketFd, buf, 4);
        if (error) {
            DPRINT("---cannot receive 4 types\n");
            fflush(stdout);
            return -1;
        }
        size = 0;
        if (1 != sscanf(buf, "%x", &size) || size <= 0) {
            DPRINT("---cannot receive message length\n");
            fflush(stdout);
            return -1;
        }
    } else {
        size = strlen(message);
    }
    char buf2[1024] = {'\0'};
    error = testReceiveMessageFromSocket(socketFd, buf2, size);
    DPRINT("===received message: %s\n", buf2);
    fflush(stdout);
    if (error) {
        return error;
    }
    if (0 == strncmp(message, buf2, size)) {
        return 0;
    } else {
        return -1;
    }
}

int testPairUpWearPhone(int adbServerSocket, int consoleServerSocket, const char* wearDevice,
        const char* phoneDevice, bool usbPhone) {
    char buf[1024] = {'\0'};
    // query regarding the watch
    int agentSocketQuery = socket_accept_any(adbServerSocket);
    if (agentSocketQuery < 0) return -1;
    snprintf(buf, sizeof(buf), "host:transport:%s", wearDevice);
    int error = testExpectMessageFromSocket(agentSocketQuery, buf);
    if (error) return error;
    error = testSendToSocket(agentSocketQuery, "OKAY");
    if (error) return error;
    const char* kProperty = "shell:getprop ro.build.version.release";
    error = testExpectMessageFromSocket(agentSocketQuery, kProperty);
    if (error) return error;
    error = testSendToSocket(agentSocketQuery, "OKAYKKWT"); // pretend to be watch
    if (error) return error;
    socket_close(agentSocketQuery);

    // query regarding the phone
    agentSocketQuery = socket_accept_any(adbServerSocket);
    if (agentSocketQuery < 0) return -1;
    snprintf(buf, sizeof(buf), "host:transport:%s", phoneDevice);
    error = testExpectMessageFromSocket(agentSocketQuery, buf);
    if (error) return error;
    error = testSendToSocket(agentSocketQuery, "OKAY");
    if (error) return error;
    error = testExpectMessageFromSocket(agentSocketQuery, kProperty);
    if (error) return error;
    error = testSendToSocket(agentSocketQuery, "OKAY4.4.2"); // pretend to be phone
    if (error) return error;
    socket_close(agentSocketQuery);

    agentSocketQuery = socket_accept_any(adbServerSocket);
    if (agentSocketQuery < 0) return -1;
    snprintf(buf, sizeof(buf), "host:transport:%s", phoneDevice);
    error = testExpectMessageFromSocket(agentSocketQuery, buf);
    if (error) return error;
    error = testSendToSocket(agentSocketQuery, "OKAY");
    if (error) return error;
    const char* kPackages = "shell:pm list packages wearablepreview";
    error = testExpectMessageFromSocket(agentSocketQuery, kPackages);
    if (error) return error;
    error = testSendToSocket(agentSocketQuery, "com.google.android.wearablepreview.app");
    if (error) return error;
    socket_close(agentSocketQuery);

    // communication between phone-console and agent
    if (usbPhone) {
        agentSocketQuery = socket_accept_any(adbServerSocket);
        if (agentSocketQuery < 0) return -1;
        char buf[1024];
        snprintf(buf, sizeof(buf), "host-serial:%s:forward:tcp:5601;tcp:5601", phoneDevice);
        error = testExpectMessageFromSocket(agentSocketQuery, buf);
        if (error) return error;
        error = testSendToSocket(agentSocketQuery, "OKAY");
        if (error) return error;
        socket_close(agentSocketQuery);
    } else {
        int consoleQuerySocket = socket_accept_any(consoleServerSocket);
        if (consoleQuerySocket < 0) {
            DPRINT("ERROR: cannot accept console port\n");
            return -1;
        }
        error = testSendToSocket(consoleQuerySocket, "Android Console\nOK\n");
        if (error) return error;
        error = testExpectMessageFromSocket(consoleQuerySocket,
                "redir add tcp:5601:5601\nquit\n", 0);
        if (error) return error;
        socket_close(consoleQuerySocket);
    }

    return 0;
}

int testStartMockServer(int* hostPort) {
    for (int i = 0; i < 100; ++i) {
        *hostPort += 1;
        DPRINT("try port: %d ...", *hostPort);
        int socket = socket_loopback_server(*hostPort, SOCKET_STREAM);
        if (socket >= 0) {
            return socket;
            DPRINT("succeeded\n");
        }
        DPRINT("failed\n");
    }
    return -1;
}

#ifndef _WIN32
static void runPairUp(const char* deviceList, int adbHostPort) {
    Looper* mainLooper = looper_newGeneric();
    {
        PairUpWearPhone pairUp(mainLooper, deviceList, adbHostPort);
        looper_run(mainLooper);
    }
    looper_free(mainLooper);
}

static int testWrapper(bool usbPhone) {
    socket_init();
    VERBOSE_DISABLE(adb);
    int adbMockServerPort = 5038; // different from adb defaut port 5037
    int adbMockServerSocket = testStartMockServer(&adbMockServerPort);
    if (adbMockServerSocket < 0) {
        DPRINT("ERROR: cannot open adb server port: %d\n", adbMockServerPort);
        return -1;
    }
    int consoleServerPort = 6556;
    int consoleServerSocket = -1;
    char phoneDevice[128] = {'\0'};
    const char kUsbPhoneDevice[] = "070fe93a0b2c1fdb";
    if (usbPhone) {
        snprintf(phoneDevice, sizeof(phoneDevice), kUsbPhoneDevice);
    } else {
        consoleServerSocket = testStartMockServer(&consoleServerPort);
        if (consoleServerSocket < 0) return -1;
        snprintf(phoneDevice, sizeof(phoneDevice), "emulator-%d", consoleServerPort);
    }
    const char kWearDevice[] = "emulator-6558";
    char deviceList[1024] = {'\0'};
    snprintf(deviceList, sizeof(deviceList), "%s device %s device", kWearDevice, phoneDevice);
    int childpid = fork();
    if (childpid == 0) {
        runPairUp(deviceList, adbMockServerPort);
        _exit(0);
    } else {
        if (childpid < 0) return -1;
    }
    int error = testPairUpWearPhone( adbMockServerSocket, consoleServerSocket, 
            kWearDevice, phoneDevice, usbPhone);
    socket_close(adbMockServerSocket);
    if (consoleServerSocket >= 0) {
        socket_close(consoleServerSocket);
    }
    return error;
}

TEST(PairUpWearPhone, TestUsbPhone) {
    int error = testWrapper(true);
    EXPECT_EQ(error, 0);
}

TEST(PairUpWearPhone, TestEmulatorPhone) {
    int error = testWrapper(false);
    EXPECT_EQ(error, 0);
}
#endif

} // namespace wear
} // namespace android
