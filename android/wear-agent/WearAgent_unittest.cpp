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
#include "android/looper.h"
#include "android/sockets.h"
#include "android/utils/debug.h"
#include "android/utils/socket_drainer.h"
#include "android/wear-agent/PairUpWearPhone_unittest.h"
#include "android/wear-agent/WearAgent.h"

#include <gtest/gtest.h>

#define  DPRINT(...)  do {  if (VERBOSE_CHECK(adb)) dprintn(__VA_ARGS__); } while (0)


namespace android {
namespace wear {
static void on_time_up (void* opaque) {
    Looper* looper=(Looper*)opaque;
    looper_forceQuit(looper);
}

// Simple test to run agent for 1 mil second and exit without crash
TEST(WearAgent, SimpleTest) {
    socket_init();
    int adbHostPort = 5038; // intentionally different from adb defaut port 5037
    int serverSocket = testStartMockServer(&adbHostPort);
    ASSERT_GE(serverSocket, 0);
    int milSecondsToRun = 1;
    Looper* mainLooper = looper_newGeneric();
    {
        LoopTimer           agentTimer[1];
        loopTimer_init(agentTimer, mainLooper, on_time_up, mainLooper);
        const Duration dl = milSecondsToRun;
        loopTimer_startRelative(agentTimer, dl);
        android::wear::WearAgent agent(mainLooper, adbHostPort);
        socket_drainer_start(mainLooper);

        looper_run(mainLooper);
        socket_drainer_drain_and_close(serverSocket);
        socket_drainer_stop();
        loopTimer_done(agentTimer);
    }
    looper_free(mainLooper);
}

#ifndef _WIN32

static int testTrackDevicesQuery(int socketFd, int& agentSocket) {
    agentSocket = socket_accept_any(socketFd);
    if (agentSocket < 0) {
        DPRINT("error happens: %d < 0\n", agentSocket);
        return -1;
    }
    int error = testExpectMessageFromSocket(agentSocket, "host:track-devices");
    return error;
}

static void runWearAgent(int adbHostPort, int milSecondsToRun) {
    Looper* mainLooper = looper_newGeneric();
    {
        LoopTimer           agentTimer[1];
        if (milSecondsToRun > 0) {
            loopTimer_init(agentTimer, mainLooper, on_time_up, mainLooper);
            const Duration dl = milSecondsToRun;
            loopTimer_startRelative(agentTimer, dl);
        }
        android::wear::WearAgent agent(mainLooper, adbHostPort);
        socket_drainer_start(mainLooper);

        looper_run(mainLooper);
        socket_drainer_stop();
        if (milSecondsToRun > 0) {
            loopTimer_done(agentTimer);
        }
    }
    DPRINT("wear agent exits\n");
    fflush(stdout);
    looper_free(mainLooper);
}

// Test that the agent should properly setup ip forwarding when there
// are one wear and one phone

static int testWearAgent(int& adbServerSocket, int& consoleServerSocket, bool usbPhone) {
    socket_init();
    VERBOSE_DISABLE(adb);
    int adbHostPort = 5038; // different from adb defaut port 5037
    adbServerSocket = testStartMockServer(&adbHostPort);
    if (adbServerSocket < 0) {
        DPRINT("ERROR: cannot open adb server port: %d\n", adbHostPort);
        return -1;
    }
    int agentSocketTracking = -1;
    int milSecondsToRun = 100;
    int childpid = fork();
    if (childpid == 0) {
        runWearAgent(adbHostPort, milSecondsToRun);
        _exit(0);
    } else {
        if (childpid < 0) return -1;
    }
    int error = testTrackDevicesQuery(adbServerSocket, agentSocketTracking);
    if (error) return error;
    error = testSendToSocket(agentSocketTracking, "OKAY");
    if (error) return error;

    char phoneDevice[128]={'\0'};
    if (usbPhone) {
        snprintf(phoneDevice, sizeof(phoneDevice), "070fe93a0b2c1fdb");
    } else {
        // start a new server at port 6556
        int consolePort = 6556;
        consoleServerSocket = testStartMockServer(&consolePort);
        if (consoleServerSocket < 0) return -1;
        snprintf(phoneDevice, sizeof(phoneDevice), "emulator-%d", consolePort);
    }
    const char kWearDevice[] = "emulator-6554";
    char kDevices[1024] = {'\0'};
    snprintf(kDevices, sizeof(kDevices), "%s device %s device", kWearDevice, phoneDevice);
    char buf[1024] = {'\0'};
    snprintf(buf, sizeof(buf), "%04x%s", (int)(strlen(kDevices)), kDevices);
    error = testSendToSocket(agentSocketTracking, buf);
    if (error) return error;

    error = testPairUpWearPhone( adbServerSocket, consoleServerSocket,
           kWearDevice, phoneDevice, usbPhone);
    return error;
}

static int testWearAgentWrapper(bool usbPhone) {
    int adbServerSocket = -1;
    int consoleServerSocket = -1;
    int error = testWearAgent(adbServerSocket, consoleServerSocket, true);
    if (adbServerSocket >= 0) {
        socket_close(adbServerSocket);
        adbServerSocket = -1;
    }
    if (consoleServerSocket >= 0) {
        socket_close(consoleServerSocket);
        consoleServerSocket = -1;
    }
    return error;
}

TEST(WearAgent, PairUpWearToUsbPhone) {
    int error = testWearAgentWrapper(true);
    EXPECT_EQ(error, 0);
}

TEST(WearAgent, PairUpWearToEmulatorPhone) {
    int error = testWearAgentWrapper(false);
    EXPECT_EQ(error, 0);
}
#endif

} // namespace wear
} // namespace android
