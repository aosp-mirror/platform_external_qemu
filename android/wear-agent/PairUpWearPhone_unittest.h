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

#ifndef ANDROID_WEAR_AGENT_PAIR_UP_WEAR_PHONE_UNIT_TEST_H
#define ANDROID_WEAR_AGENT_PAIR_UP_WEAR_PHONE_UNIT_TEST_H

namespace android {
namespace wear {

int testPairUpWearPhone(int adbServerSocket, int consoleServerSocket,
        const char* wearDevice, const char* phoneDevice, bool usbPhone);
int testSendToSocket(int socketFd, const char* message);
int testReceiveMessageFromSocket(int socketFd, char* ptr, int size);
int testExpectMessageFromSocket(int socketFd, const char* message, int header=1);
int testStartMockServer(int* hostPort);

} // namespace wear
} // namespace android

#endif
