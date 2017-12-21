// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/emulation/AdbMessageSniffer.h"

#include "android/globals.h"

#include <atomic>

static std::atomic<int> host_cts_heart_beat_count {};

namespace android {
namespace emulation {

// the following definition is defined in system/core/adb

#define ADB_SYNC 0x434e5953
#define ADB_CNXN 0x4e584e43
#define ADB_OPEN 0x4e45504f
#define ADB_OKAY 0x59414b4f
#define ADB_CLSE 0x45534c43
#define ADB_WRTE 0x45545257
#define ADB_AUTH 0x48545541

#define ADB_AUTH_TOKEN         1
#define ADB_AUTH_SIGNATURE     2
#define ADB_AUTH_RSAPUBLICKEY  3
#define ADB_VERSION 0x01000000
// 1 MB
#define MAX_ADB_MESSAGE_PAYLOAD 1048576

AdbMessageSniffer::AdbMessageSniffer(const char* name):mName(name) {
    memset(&mPacket, 0, sizeof(apacket));
    mBufferVec.resize(MAX_ADB_MESSAGE_PAYLOAD);
    startNewMessage();
}

AdbMessageSniffer::~AdbMessageSniffer() {
}

void AdbMessageSniffer::grow_buffer_if_needed(int count) {
    if (mBufferVec.size() < count) {
        mBufferVec.resize(2 * count);
    }
    mBuffer = static_cast<uint8_t*> (&(mBufferVec[0]));
}

void AdbMessageSniffer::readToBuffer(const AndroidPipeBuffer* buffers, int numBuffers, int count) {

    mBufferP = mBuffer;

    for (int i=0; i < numBuffers; ++i) {
        uint8_t* data = buffers[i].data;
        int dataSize = buffers[i].size;
        if (count < dataSize) {
            memcpy(mBufferP, data, count);
            return;
        } else {
            memcpy(mBufferP, data, dataSize);
            mBufferP += dataSize;
            count -= dataSize;
            if (count <= 0) {
                return;
            }
        }
    }
}

void AdbMessageSniffer::parseBuffer(int bufferLength) {
    mBufferP = mBuffer;
    int count = bufferLength;
    while (count > 0) {
        if (mState == 0) {
            count -= readHeader(count);
        } else {
            count -= readPayload(count);
        }
    }
}

int AdbMessageSniffer::readHeader(int dataSize) {
    CHECK( mPacket.data - mCurrPos > 0);
    int need = sizeof(amessage) - (mCurrPos - (uint8_t*)(&mPacket));
    if (need > dataSize) {
        copyFromBuffer(dataSize);
        return dataSize;
    } else {
        copyFromBuffer(need);
        updateCtsHeartBeatCount();
        printMessage();
        mState = 1;
        return need;
    }
}

void AdbMessageSniffer::updateCtsHeartBeatCount() {
    amessage & msg = mPacket.mesg;
    // OKAY is usually from logcat, ignore it
    if (msg.command != ADB_OKAY) {
        host_cts_heart_beat_count += 1;
    }
}

void AdbMessageSniffer::checkForShellExit() {
    amessage & msg = mPacket.mesg;
    if (msg.command == ADB_OPEN) {
        mPacket.data[msg.data_length] = '\0';
        const char *purpose = (char*)(mPacket.data);
        if (strncmp(purpose, "shell", 5)==0) {
            if (strstr(purpose, ":exit") != NULL) {
                //found shell exit
                printf("found shell %s\n", purpose);
            }
        }
    }
}

int AdbMessageSniffer::readPayload(int dataSize) {
    CHECK(mCurrPos - mPacket.data >= 0);
    int need = getPayloadSize() - (mCurrPos - mPacket.data);
    if (need > dataSize) {
        copyFromBuffer(dataSize);
        return dataSize;
    } else {
        copyFromBuffer(need);
        checkForShellExit();
        printPayload();
        startNewMessage();
        return need;
    }
}

void AdbMessageSniffer::read(const AndroidPipeBuffer* buffers, int numBuffers, int count) {
    if (count <= 0) return;
    grow_buffer_if_needed(count);
    readToBuffer(buffers, numBuffers, count);
    parseBuffer(count);
}

int AdbMessageSniffer::getPayloadSize() {
    return mPacket.mesg.data_length;
}

void AdbMessageSniffer::copyFromBuffer(int count) {
    // guard against overflow
    int avail = sizeof(mPacket) - (mCurrPos - mPacket.data) - sizeof(mPacket.mesg);
    if (avail > 0 ) {
        int size = std::min(avail, count);
        memcpy(mCurrPos, mBufferP, size);
    }

    mCurrPos += count;
    mBufferP += count;
}

int AdbMessageSniffer::getAllowedBytesToPrint(int bytes) {
    int allowed = std::min(bytes, (int)(sizeof(mPacket.data)));
    return allowed;
}

void AdbMessageSniffer::printPayload() {
    if (android_hw->test_monitorAdb< 2 ) {
        return;
    }
    amessage & msg = mPacket.mesg;
    if (   msg.command == ADB_OKAY
        || msg.command == ADB_WRTE) {
        return;
    }
    int length = msg.data_length;
    length = getAllowedBytesToPrint(length);
    if (length <= 0 ) return;
    uint8_t* data = mPacket.data;
    for (int i=0; i < length; ++i) {
        if (i % 1024 == 0) {
            printf("%s:", mName);
        }
        int ch = data[i];
        if (isprint(ch)) {
            printf("%c", ch);
        } else if (isspace(ch)) {
            //printf("%c", ch);
        } else {
            //printf(" ");
        }
    }
    printf("\n");
}

const char* AdbMessageSniffer::getCommandName(unsigned code) {
    switch(code) {
        case ADB_SYNC:
            return "SYNC";
        case ADB_CNXN:
            return "CNXN";
        case ADB_OPEN:
            return "OPEN";
        case ADB_CLSE:
            return "CLOSE";
        case ADB_AUTH:
            return "AUTH";
        case ADB_WRTE:
            return "WRITE";
        case ADB_OKAY:
            return "OKAY";
        default:
            return "unknown";
    }
}

void AdbMessageSniffer::printMessage() {
    if (android_hw->test_monitorAdb < 2 ) {
        return;
    }
    amessage & msg = mPacket.mesg;
    if (   msg.command == ADB_OKAY
        || msg.command == ADB_WRTE) {
        return;
    }

    printf("%s:command: %s ", mName, getCommandName(msg.command));
    printf("arg0: %d ", msg.arg0);
    printf("arg1: %d ", msg.arg1);
    printf("data length: %d\n", msg.data_length);
}

void AdbMessageSniffer::startNewMessage() {
    mCurrPos = (uint8_t*)(&mPacket);
    mState = 0;
}

}  // namespace emulation
}  // namespace android

extern "C" int get_host_cts_heart_beat_count(void) {
    return host_cts_heart_beat_count.load();
}

