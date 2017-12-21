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

AdbMessageSniffer::AdbMessageSniffer(const char* name):mName(name) {
    memset(&mPacket, 0, sizeof(apacket));
    startNewMessage();
}

AdbMessageSniffer::~AdbMessageSniffer() {
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
            if (count == 0) {
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
        registerMessage();
        printMessage();
        mState = 1;
        return need;
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
                mPrintQuota.erase(msg.arg0);
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
    readToBuffer(buffers, numBuffers, count);
    parseBuffer(count);
}

int AdbMessageSniffer::getPayloadSize() {
    return mPacket.mesg.data_length;
}

void AdbMessageSniffer::copyFromBuffer(int count) {
    memcpy(mCurrPos, mBufferP, count);
    mCurrPos += count;
    mBufferP += count;
}

int AdbMessageSniffer::getAllowedBytesToPrint(int bytes) {
    amessage & msg = mPacket.mesg;
    if (msg.command == ADB_CLSE) return bytes;
    if (mPrintQuota.find(msg.arg0) == mPrintQuota.end()) {
        return 0;
    }
    if (mPrintQuota[msg.arg0] <= 0) {
        //too much spam
        return 0;
    }

    int allowed = std::min(bytes, mPrintQuota[msg.arg0]);
    mPrintQuota[msg.arg0] -= allowed;
    return allowed;
}

void AdbMessageSniffer::printPayload() {
    amessage & msg = mPacket.mesg;
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
            printf("%c", ch);
        } else {
            printf(" ");
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

void AdbMessageSniffer::registerMessage() {
    amessage & msg = mPacket.mesg;
    if (msg.command == ADB_OPEN) {
        printf("register open %d\n", msg.arg0);
        mPrintQuota[msg.arg0] = 1024;
    } else if (msg.command == ADB_CLSE) {
        printf("un register %d\n", msg.arg0);
        mPrintQuota.erase((msg.arg0));
    } else if (mPrintQuota.find(msg.arg0) == mPrintQuota.end()) {
        printf("first time register %s\n", getCommandName(msg.command));
        mPrintQuota[msg.arg0] = 1024;
    }
}

void AdbMessageSniffer::printMessage() {
    amessage & msg = mPacket.mesg;
    int allowed = getAllowedBytesToPrint(24);
    if (allowed <= 0) return;

    printf("%s:command: %s ", mName, getCommandName(msg.command));
    printf("arg0: %d ", msg.arg0);
    printf("arg1: %d ", msg.arg1);
    printf("data length: %d\n", msg.data_length);
}

void AdbMessageSniffer::startNewMessage() {
    mCurrPos = (uint8_t*)(&mPacket);
    mState = 0;
    mBufferP = mBuffer;
}

}  // namespace emulation
}  // namespace android
