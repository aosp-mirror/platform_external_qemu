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

#include "android/base/ArraySize.h"
#include "android/globals.h"

#include <atomic>

static std::atomic<int> host_cts_heart_beat_count{};

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

#define ADB_AUTH_TOKEN 1
#define ADB_AUTH_SIGNATURE 2
#define ADB_AUTH_RSAPUBLICKEY 3
#define ADB_VERSION 0x01000000
// Maximum payload for latest adb version (see system/core/adb/adb.h)
constexpr size_t MAX_ADB_MESSAGE_PAYLOAD = 1024 * 1024;

AdbMessageSniffer::AdbMessageSniffer(const char* name,
                                     int level,
                                     std::ostream* oStream)
    : mName(name), mLevel(level), mLogStream(oStream) {
    memset(&mPacket, 0, sizeof(apacket));
    startNewMessage();
}

AdbMessageSniffer::~AdbMessageSniffer() {}

void AdbMessageSniffer::grow_buffer_if_needed(int count) {
    if (mBufferVec.size() < count) {
        mBufferVec.resize(2 * count);
    }
    mBuffer = static_cast<uint8_t*>(&(mBufferVec[0]));
}

void AdbMessageSniffer::readToBuffer(const AndroidPipeBuffer* buffers,
                                     int numBuffers,
                                     int count) {
    mBufferP = mBuffer;

    for (int i = 0; i < numBuffers; ++i) {
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
            int rd = readHeader(count);
            if (rd < 0)
                return;
            count -= rd;
        } else {
            count -= readPayload(count);
        }
    }
}

int AdbMessageSniffer::readHeader(int dataSize) {
    CHECK(mPacket.data - mCurrPos > 0);
    auto need = sizeof(amessage) - (mCurrPos - (uint8_t*)(&mPacket));
    if (need > dataSize) {
        copyFromBuffer(dataSize);
        return dataSize;
    } else {
        copyFromBuffer(need);
        if (!packetSeemsValid()) {
            *mLogStream << mName
                        << " Received invalid packet.. Disabling logging.";
            mLevel = 0;
            return -1;
        }

        printMessage();
        amessage& msg = mPacket.mesg;
        if (msg.command == ADB_CLSE) {
            mDummyShellArg0.erase(msg.arg0);
        }
        mState = 1;
        return need;
    }
}

void AdbMessageSniffer::updateCtsHeartBeatCount() {
    amessage& msg = mPacket.mesg;
    // OKAY is usually from logcat, ignore it
    if (msg.command != ADB_OKAY && msg.command != ADB_CLSE) {
        if (!checkForDummyShellCommand()) {
            host_cts_heart_beat_count += 1;
            if (mLevel >= 2) {
                *mLogStream << "emulator: cts heartbeat "
                            << host_cts_heart_beat_count.load() << std::endl;
                mLogStream->flush();
            }
        }
    }
}

bool AdbMessageSniffer::checkForDummyShellCommand() {
    amessage& msg = mPacket.mesg;
    if (msg.command == ADB_OPEN) {
        mPacket.data[msg.data_length] = '\0';
        const char* purpose = (char*)(mPacket.data);
        const char* s_dummy_shell_commands[] = {
                "shell:exit", "shell:getprop",
                "shell:cat /sys/class/power_supply/*/capacity", "framebuffer"};
        // older version
        // shell:getprop
        for (int ii = 0; ii < ARRAY_SIZE(s_dummy_shell_commands); ++ii) {
            const char* dummy = s_dummy_shell_commands[ii];
            if (strncmp(purpose, dummy, strlen(dummy)) == 0) {
                mDummyShellArg0.insert(msg.arg0);
                return true;
            }
        }
        // newer version
        // shell,v2,TERM=xterm-256color,raw:getprop
        const char* s_dummy_patterns[] = {
                ",raw:exit", ",raw:getprop",
                ",raw:cat /sys/class/power_supply/*/capacity"};
        if (strncmp(purpose, "shell,", 6) == 0) {
            for (int jj = 0; jj < ARRAY_SIZE(s_dummy_patterns); ++jj) {
                const char* dummy = s_dummy_patterns[jj];
                if (strstr(purpose, dummy) != NULL) {
                    mDummyShellArg0.insert(msg.arg0);
                    return true;
                }
            }
        }
    } else if (msg.command == ADB_WRTE &&
               mDummyShellArg0.find(msg.arg0) != mDummyShellArg0.end()) {
        return true;
    }
    return false;
}

int AdbMessageSniffer::readPayload(int dataSize) {
    CHECK(mCurrPos - mPacket.data >= 0);

    auto need = getPayloadSize() - (mCurrPos - mPacket.data);
    if (need > dataSize) {
        copyFromBuffer(dataSize);
        return dataSize;
    } else {
        copyFromBuffer(need);
        updateCtsHeartBeatCount();
        printPayload();
        startNewMessage();
        return need;
    }
}

bool AdbMessageSniffer::packetSeemsValid() {
    // Here you can add a series of checks to validate a packet.
    return (mPacket.mesg.command == (mPacket.mesg.magic ^ 0xffffffff)) &&
           (mPacket.mesg.data_length <= MAX_ADB_MESSAGE_PAYLOAD);
}

void AdbMessageSniffer::read(const AndroidPipeBuffer* buffers,
                             int numBuffers,
                             int count) {
    if (count <= 0 || mLevel < 1)  // We only track if we are logging..
        return;
    grow_buffer_if_needed(count);
    readToBuffer(buffers, numBuffers, count);
    parseBuffer(count);
}

size_t AdbMessageSniffer::getPayloadSize() {
    return mPacket.mesg.data_length;
}

void AdbMessageSniffer::copyFromBuffer(size_t count) {
    // guard against overflow
    size_t avail =
            sizeof(mPacket) - (mCurrPos - mPacket.data) - sizeof(mPacket.mesg);
    size_t size = 0;
    if (avail > 0) {
        size = std::min<size_t>(avail, count);
        memcpy(mCurrPos, mBufferP, size);
    }

    mCurrPos += size;
    mBufferP += size;
}

int AdbMessageSniffer::getAllowedBytesToPrint(int bytes) {
    int allowed = std::min<int>(bytes, sizeof(mPacket.data));
    return allowed;
}

void AdbMessageSniffer::printPayload() {
    if (mLevel < 2) {
        return;
    }
    amessage& msg = mPacket.mesg;
    if (msg.command == ADB_OKAY)
        return;
    if ((msg.command == ADB_WRTE) && (mLevel < 3))
        return;
    int length = msg.data_length;
    length = getAllowedBytesToPrint(length);
    if (length <= 0)
        return;
    uint8_t* data = mPacket.data;
    for (int i = 0; i < length; ++i) {
        if (i % 1024 == 0) {
            *mLogStream << mName << " ";
        }
        int ch = data[i];
        if (isprint(ch)) {
            *mLogStream << (char)ch;
        } else if (isspace(ch)) {
            // printf("%c", ch);
        } else {
            // printf(" ");
        }
    }
    *mLogStream << std::endl;
}

const char* AdbMessageSniffer::getCommandName(unsigned code) {
    switch (code) {
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
    if (mLevel < 2) {
        return;
    }

    amessage& msg = mPacket.mesg;
    if (msg.command == ADB_OKAY)
        return;
    if ((msg.command == ADB_WRTE) && (mLevel < 3))
        return;

    *mLogStream << mName << " command: " << getCommandName(msg.command)
                << ", arg0: " << msg.arg0 << ", arg1: " << msg.arg1
                << ", data length:" << msg.data_length << std::endl;
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
